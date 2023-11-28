/*
 *  linux/fs/befs/namei.c
 *
 * Copyright (C) 1999  Makoto Kato (m_kato@ga2.so-net.ne.jp)
 *
 *  from
 *
 *  linux/fs/minix/namei.c
 *
 *  Copyright (C) 1991, 1992  Linus Torvalds
 */

#include <asm/uaccess.h>

#include <linux/errno.h>
#include <linux/fs.h>
#include <linux/befs_fs.h>
#include <linux/fcntl.h>
#include <linux/sched.h>
#include <linux/stat.h>
#include <linux/string.h>
#include <linux/locks.h>
#include <linux/quotaops.h>


/*
 * befs_find_entry
 *
 * description:
 *  find file or directry entry
 *
 * parameter:
 *  dir     ... parent's inode
 *  name    ... file or directry name
 *  namelen ... length of name
 */

static befs_off_t befs_find_entry (struct inode * dir, const char * const name,
	int namelen)
{
	struct buffer_head *    bh;
	befs_index_node *        bn;
	struct super_block *    sb = dir->i_sb;
	int                     sd_pos = 0;
	int                     key_pos;
	char                    key[BEFS_NAME_LEN + 1];
	int                     key_len;
	befs_off_t               value;

	BEFS_OUTPUT (("---> befs_find_entry()\n"));

	while (sd_pos >= 0) {
		befs_data_stream * ds = &dir->u.befs_i.i_data.ds;
		befs_inode_addr    iaddr;
		int               flags;
		befs_off_t         offset;

		/*
		 * get index node from small_data.
		 *
		 * First data stream has index entry, but other than it,
		 * doesn't have it.  
		 */

		if (sd_pos)
			flags = 1;
		else
			flags = 0;

		iaddr = befs_read_data_stream (sb, ds, &sd_pos);
		if (sd_pos <= 0) {
			/*
			 * cannot read next data stream
			 */

			BEFS_OUTPUT (("<--- befs_find_entry() "
				"cannot read next data stream\n"));
			return 0;
		}

		bh = befs_read_index_node (iaddr, sb, flags, &offset);
		if (!bh) {

			/*
			 * cannot read index node
			 */

			BEFS_OUTPUT (("<--- befs_find_entry() "
				"cannot read index node\n"));
			return 0;
		}

		bn = (befs_index_node *) (bh->b_data + offset);

		/*
		 * explore in this block
		 */

		key_pos = 0;
		while (key_pos >= 0) {

			befs_get_key_from_index_node (bn, &key_pos,
				BEFS_TYPE(sb), key, &key_len, &value);
			if (key_pos < 0) {
				BEFS_OUTPUT ((" not exist in this block\n"));
				break;
			}

			if (!strcmp (key, name)) {
				brelse (bh);

				BEFS_OUTPUT (("<--- befs_find_entry() "
					"value = %Lu\n", value));
				return value;
			}
		}

		brelse (bh);
	}

	BEFS_OUTPUT (("<--- befs_find_entry() not found\n"));
	return 0;
}



int befs_lookup (struct inode * dir, struct dentry *dentry)
{
	struct inode * inode = NULL;
	befs_off_t      offset;
	int            len;
	char *         tmpname;

	BEFS_OUTPUT (("---> befs_lookup() "
		"name %s inode %ld\n", dentry->d_name.name, dir->i_ino));

	/*
	 * Convert to UTF-8
	 */

	if (!befs_nls2utf (dentry->d_name.name, dentry->d_name.len, &tmpname,
		&len, dir->i_sb)) {

		return -ENOMEM;
	}

	if (len > BEFS_NAME_LEN) {
		putname (tmpname);
		return -ENAMETOOLONG;
	}

	offset = befs_find_entry (dir, tmpname, len);
	putname (tmpname);

	if (offset) {
		inode = iget (dir->i_sb, (ino_t) offset);
		if (!inode)
			return -EACCES;
	}

	d_add (dentry, inode);

	BEFS_OUTPUT (("<--- befs_lookup()\n"));

	return 0;
}
