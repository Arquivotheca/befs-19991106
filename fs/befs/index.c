/*
 *  linux/fs/befs/index.c
 *
 * Copyright (C) 1999  Makoto Kato (m_kato@ga2.so-net.ne.jp)
 *
 *
 *  Index functions for BEFS.
 */

#include <asm/uaccess.h>

#include <linux/errno.h>
#include <linux/fs.h>
#include <linux/befs_fs.h>
#include <linux/sched.h>
#include <linux/stat.h>
#include <linux/mm.h>


static void befs_convert_index_entry (int, befs_index_entry *,
	befs_index_entry *);

/*
 * Convert little-edian and big-edian.
 */

static void befs_convert_index_entry (int fstype, befs_index_entry * entry,
	befs_index_entry * out)
{
	switch (fstype)
	{
	case BEFS_X86:
		out->magic = le32_to_cpu(entry->magic);
		out->node_size = le32_to_cpu(entry->node_size);
		out->max_number_of_levels =
			le32_to_cpu(entry->max_number_of_levels);
		out->data_type = le32_to_cpu(entry->data_type);
		out->root_node_pointer =
			le64_to_cpu(entry->root_node_pointer);
		out->free_node_pointer =
			le64_to_cpu(entry->free_node_pointer);
		out->maximum_size = le64_to_cpu(entry->maximum_size);
		break;

	case BEFS_PPC:
		out->magic = be32_to_cpu(entry->magic);
		out->node_size = be32_to_cpu(entry->node_size);
		out->max_number_of_levels =
			be32_to_cpu(entry->max_number_of_levels);
		out->data_type = be32_to_cpu(entry->data_type);
		out->root_node_pointer =
			be64_to_cpu(entry->root_node_pointer);
		out->free_node_pointer =
			be64_to_cpu(entry->free_node_pointer);
		out->maximum_size = be64_to_cpu(entry->maximum_size);
		break;
	}
}


void befs_convert_index_node (int fstype, befs_index_node * node,
	befs_index_node * out)
{
	switch (fstype)
	{
	case BEFS_X86:
		out->left = le64_to_cpu (node->left);
		out->right = le64_to_cpu (node->right);
		out->overflow = le64_to_cpu (node->overflow);
		out->all_key_count = le16_to_cpu (node->all_key_count);
		out->all_key_length = le16_to_cpu (node->all_key_length);
		break;

	case BEFS_PPC:
		out->left = be64_to_cpu (node->left);
		out->right = be64_to_cpu (node->right);
		out->overflow = be64_to_cpu (node->overflow);
		out->all_key_count = be16_to_cpu (node->all_key_count);
		out->all_key_length = be16_to_cpu (node->all_key_length);
		break;
	}
}


/*
 * befs_read_index_node
 *
 * description:
 *  read index node from inode.
 *
 * parameter:
 *  inode  ... address of index node
 *  sb     ... super block
 *  flags  ... 0 ... inode is index entry
 *             1 ... inode is index node
 *  offset ... offset into index address
 */

struct buffer_head * befs_read_index_node (befs_inode_addr inode,
	struct super_block * sb, int flags, befs_off_t * offset)
{
	struct buffer_head * bh;
	befs_inode_addr       iaddr;
	befs_index_entry *    entry;

	BEFS_OUTPUT (("---> befs_read_index_node() "
		"index [%lu, %u, %u]\n",
		inode.allocation_group, inode.start, inode.len));

	if (!flags) {

		/*
		 * read index entry
		 */

		bh = befs_bread2 (sb, inode);
		if (!bh) {
			printk (KERN_ERR "BEFS: cannot read index entry.\n");
			return NULL;
		}
#ifdef CONFIG_BEFS_CONV
		entry = (befs_index_entry *) __getname();;
		if (!entry) {
			printk (KERN_ERR "BEFS: cannot allocate memory\n");
			return NULL;
		}

		/*
		 * Convert little-edian and big-edian.
		 */

		befs_convert_index_entry (sb->u.befs_sb.mount_opts.befs_type,
			(befs_index_entry *) bh->b_data, entry);
#else
		entry = (befs_index_entry *) bh->b_data;
#endif
		BEFS_DUMP_INDEX_ENTRY (entry);

		/*
		 * check magic header
		 */

		if (entry->magic != BEFS_INDEX_MAGIC) {

			/*
			 * bad magic header
			 */

			printk (KERN_ERR "BEFS: "
				"magic header of index entry is bad value.\n");
#ifdef CONFIG_BEFS_CONV
			putname (entry);
#endif
			brelse(bh);

			return NULL;
		}

		/*
		 * set index entry postion
		 */

		*offset = 0;
		iaddr = inode;
		if (entry->node_size < sb->u.befs_sb.block_size) {

			/*
			 * exist index node into index entry's block
			 */

			*offset = entry->node_size;
		} else {
			iaddr.start += entry->node_size
				/ sb->u.befs_sb.block_size;
			iaddr.len -= entry->node_size / sb->u.befs_sb.block_size;
			*offset = entry->node_size % sb->u.befs_sb.block_size;
		}

#ifdef CONFIG_BEFS_CONV
		putname (entry);
#endif
		brelse(bh);

	} else {
		BEFS_OUTPUT ((" skip index node\n"));
		iaddr = inode;
	}

	BEFS_OUTPUT ((" inode of index node\n"));
	BEFS_DUMP_INODE_ADDR (iaddr);

	bh = befs_bread2 (sb, iaddr);

	BEFS_OUTPUT (("<--- befs_read_index_node() %s offset = %016x\n",
		(bh ? "success" : "fail"), *offset));

	return bh;
}


/*
 * befs_get_key_from_index_node
 *
 * description:
 *  Get key form index node
 *
 *  parameter:
 *   node   ... index node structure (raw_disk format)
 *   pos    ... key position
 *   fstype ... filesystem type
 *   name   ... keyname return
 *   len    ... length of keyname return.
 *   iaddr  ... key valude return.
 */

char * befs_get_key_from_index_node (befs_index_node * node, int *pos, int fstype,
	char * name, int * len, befs_off_t * iaddr)
{
	char *           key;
	__u16 *          key_array;
	befs_off_t *      key_value;
	int              key_offset;
	char *           namep = NULL;
	befs_index_node * bn;

	BEFS_OUTPUT (("---> befs_get_key_from_index_node() "
		"pos %d\n", *pos));

	/*
	 * Convert little endian and big endian.
	 */

#ifdef CONFIG_BEFS_CONV
	bn = (befs_index_node *) __getname();
	if (!bn)
		return NULL;

	befs_convert_index_node (fstype, node, bn);
#else
	bn = node;
#endif

	if (*pos < 0 || bn->all_key_count <= *pos) {
		*pos = -1;
#ifdef CONFIG_BEFS_CONV
		putname(bn);
#endif
		return NULL;
	}

	/*
	 * Only key_array, byte is 8byte...
	 */

	key = (char*) node + sizeof(befs_index_node);
	key_offset = (bn->all_key_length + sizeof(befs_index_node)) % 8;
	key_array = (__u16 *) (key + bn->all_key_length
		+ (!key_offset ? 0 : 8 - key_offset));
	key_value = (befs_off_t *) (((char *) key_array) + sizeof(__u16)
		* bn->all_key_count);
#ifdef BEFS_DEBUG
	{
		int j;
		char key_test[4096];

		memcpy (key_test, key, bn->all_key_length);
		key_test[bn->all_key_length] = '\0';

		BEFS_OUTPUT ((" key content %s\n", key_test));

		for (j = 0; j < *pos; j++) {
			BEFS_OUTPUT ((" kay_array[%d] = %d  ",
				j, key_array[j]));
			BEFS_OUTPUT (("kay_value[%d] = %d\n", j, key_value[j]));
		}
	}
#endif
#ifdef CONFIG_BEFS_CONV
	switch (fstype)
	{
	case BEFS_X86:
		if (*pos == 0) {
			namep = key;
			*len = le16_to_cpu(key_array[*pos]);
		} else {
			namep = key + le16_to_cpu(key_array[*pos - 1]);
			*len = le16_to_cpu(key_array[*pos])
				- le16_to_cpu(key_array[*pos - 1]);
		}
		*iaddr = le64_to_cpu(key_value[*pos]);
		break;

	case BEFS_PPC:
		if (*pos == 0) {
			namep = key;
			*len = be16_to_cpu(key_array[*pos]);
		} else {
			namep = key + be16_to_cpu(key_array[*pos - 1]);
			*len = be16_to_cpu(key_array[*pos])
				- be16_to_cpu(key_array[*pos - 1]);
		}
		*iaddr = be64_to_cpu(key_value[*pos]);
		break;
	default:
		/*
		 * Unknown filesystem type
		 */
		putname(bn);
		return NULL;
	}
#else
	if (!*pos) {
		namep = key;
		*len = key_array[*pos];
	} else {
		namep = key + key_array[*pos - 1];
		*len = key_array[*pos] - key_array[*pos - 1];
	}

	*iaddr = key_value[*pos];
#endif

	/*
	 * copy from namep to name
	 */

	memcpy (name, namep, *len);
	name[*len] = '\0';
	(*pos)++;

#ifdef CONFIG_BEFS_CONV
	putname (bn);
#endif

	BEFS_OUTPUT (("<--- befs_get_key_from_index_node() "
		"result key [%s], value %u\n", name, *iaddr));

	return name;
}



#ifdef CONFIG_BEFS_RW
/*
 * befs_is_key_into_index
 *
 */

int befs_is_key_into_index (struct super_block * sb,
	befs_inode_addr index_iaddr, int flags, char * keyname, int * rpos)
{
	int                   pos;
	char *                tmpname = NULL;
	int                   len;
	befs_index_node *      node = NULL;
	struct buffer_head *  bh = NULL;
	befs_index_node *      bn = NULL;
	befs_off_t             offset;
	befs_off_t             value;

	if (BEFS_IS_EMPTY_IADDR(&index_iaddr))
		return -EBADF;

	BEFS_OUTPUT (("---> befs_delete_into_index [%d, %d, %d]",
		index_iaddr.allocation_group, index_iaddr.start,
		index_iaddr.len));

	bh = befs_read_index_node (index_iaddr, sb, flags, &offset);
	if (!bh)
		return -EBADF;

	bn = (befs_index_node *) (bh->b_data + offset);

#ifdef CONFIG_BEFS_CONV
	node = (befs_index_node *) __getname();
	if (!node) {
		brelse (bh);

		return -ENOMEM;
	}

	befs_convert_index_node (BEFS_TYPE(sb), bn, node);
#else
	node = bn;
#endif

	tmpname = (char *) __getname();
	if (!tmpname) {
		brelse (bh);
#ifdef CONFIG_BEFS_CONV
		putname (node);
#endif
		return -ENOMEM;
	}

	pos = 0;
	for (pos = 0; pos < node->all_key_count; ) {
		if (!befs_get_key_from_index_node (bn, &pos, BEFS_TYPE(sb),
			tmpname, &len, &value)) {

			if (pos != -1) {
				/*
				 * error
				 */

				brelse (bh);
				putname (tmpname);
#ifdef CONFIG_BEFS_CONV
				putname (node);
#endif

				return -EBADF;
			} else {
				/*
				 * not exit key into this block
				 */

				break;
			}
		}

		if (!strcmp (tmpname, keyname)) {
			continue;
		} else {
			/*
			 * found index node
			 */
			BEFS_OUTPUT (("  found index node! [%s]\n", keyname));

			*rpos = pos;

			brelse (bh);
			putname (tmpname);
#ifdef CONFIG_BEFS_CONV
			putname (node);
#endif

			return 0;
		}
	}

	/*
	 * cannot find index node
	 */

	*rpos = -1;

	brelse (bh);
	putname (tmpname);
#ifdef CONFIG_BEFS_CONV
	putname (node);
#endif

	return 0;
}	

int befs_delete_index_node (struct super_block * sb, befs_inode_addr iaddr,
	char * name)
{
	int pos = 0;
	int flags = 1;
	struct buffer_head *  bh;

	/*
	 * exist key name ?
	 */

	while (iaddr.len > 0) {
		err = befs_is_key_into_index (sb, iaddr, flags, name, &pos);
		if (err)
			return err;
		else if (pos != -1) {
			/*
			 * found key into this block.
			 */

			break;
		}

		iaddr.start++;
		iaddr.len--;
		flags = 0;
	}

	if (i >= addr.len)
		return -EBADF;

	/*
	 * get index node
	 */

	bh = befs_read_index_node (index_iaddr, sb, flags, &offset);
	if (!bh)
		return -EBADF;

	bn = (befs_index_node *) (bh->b_data + offset);

#ifdef CONFIG_BEFS_CONV
	node = (befs_index_node *) __getname();
	if (!node) {
		brelse (bh);

		return -ENOMEM;
	}

	befs_convert_index_node (BEFS_TYPE(sb), bn, node);
#else
	node = bn;
#endif
}
#endif
