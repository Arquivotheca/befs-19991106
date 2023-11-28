/*
 *  linux/fs/befs/dir.c
 *
 * Copyright (C) 1999  Makoto Kato (m_kato@ga2.so-net.ne.jp)
 *
 *  from
 *
 *  linux/fs/minix/dir.c
 *
 *  Copyright (C) 1991, 1992  Linus Torvalds
 */

#include <asm/uaccess.h>

#include <linux/errno.h>
#include <linux/fs.h>
#include <linux/befs_fs.h>
#include <linux/sched.h>
#include <linux/stat.h>
#include <linux/mm.h>
#include <linux/nls.h>


static ssize_t befs_dir_read (struct file * filp, char * buf, size_t count,
	loff_t *ppos)
{
	return -EISDIR;
}


static int befs_readdir(struct file *, void *, filldir_t);

static struct file_operations befs_dir_operations = {
	NULL,			/* lseek - default */
	befs_dir_read,		/* read */
	NULL,			/* write - bad */
	befs_readdir,		/* readdir */
	NULL,			/* poll - default */
	NULL,			/* ioctl */
	NULL,			/* mmap */
	NULL,			/* no special open code */
	NULL,			/* flush */
	NULL,			/* no special release code */
	file_fsync,		/* fsync */
	NULL,			/* fasync */
	NULL,			/* check_media_change */
	NULL			/* revalidate */
};


/*
 * directories can handle most operations...
 */

struct inode_operations befs_dir_inode_operations = {
	&befs_dir_operations,	/* default directory file-ops */
	NULL,			/* create */
	befs_lookup,		/* lookup */
	NULL,			/* link */
	NULL,			/* unlink */
	NULL,			/* symlink */
	NULL,			/* mkdir */
	NULL,			/* rmdir */
	NULL,			/* mknod */
	NULL,			/* rename */
	NULL,			/* readlink */
	NULL,			/* follow_link */
	NULL,			/* readpage */
	NULL,			/* writepage */
#if LINUX_VERSION_CODE >= 0x020309
	NULL,			/* get_block */
#else
	NULL,			/* bmap */
#endif
	NULL,			/* truncate */
	NULL,			/* permission */
	NULL			/* smap */
};


/*
 * description:
 *  A directry structure of BEFS become index structure.
 *
 *  sturct befs_index_node
 *  {
 *   off_t  left_link;
 *   off_t  right_link;
 *   off_t  overflow_link;
 *   short  all_node_count;
 *   short  all_key_length
 *   byte   key_data[all_key_length]
 *   short  key_index_count[all_node_count - 1]
 *   off_t  values[all_node_count]
 *  }
 *
 *  directry have above key.
 *  [.] [..] [file and directry] ...
 */

static int befs_readdir(struct file * filp, void * dirent, filldir_t filldir)
{
	struct inode *       inode = filp->f_dentry->d_inode;
	struct super_block * sb = inode->i_sb;
	befs_inode_addr       iaddr;
	char *               tmpname;
	int                  error;
	int                  count = 0;
	int                  len;
	befs_data_stream *    ds;
	int                  pos = 0;

	BEFS_OUTPUT (("---> befs_readdir() "
		"inode %ld filp->f_pos %lu\n",
		inode->i_ino, filp->f_pos));

	if (!inode || !S_ISDIR(inode->i_mode) || S_ISLNK(inode->i_mode))
		return -EBADF;

	tmpname = (char *) __getname();
	if (!tmpname)
		return -ENOMEM;

	ds = &inode->u.befs_i.i_data.ds;

	while (pos >= 0) {

		struct buffer_head * bh;
		befs_index_node *     bn;
		befs_index_node *     node;
		befs_off_t            value;
		int                  flags;
		int                  k;
		befs_off_t            offset;
		char *               tmpname2;
		int                  len_dist;
		
		if (pos)
			flags = 1;
		else
			flags = 0;

		iaddr = befs_read_data_stream (sb, ds, &pos);
		if (!pos || BEFS_IS_EMPTY_IADDR(&iaddr))
			break;

		/*
		 * read index node
		 */

		bh = befs_read_index_node (iaddr, sb, flags, &offset);
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
		BEFS_DUMP_INDEX_NODE (node);

		/*
		 * Is there no directry key in this index node?
		 */

		if (node->all_key_count + count <= filp->f_pos) {

			/*
			 * Not exist
			 */

			count += node->all_key_count;
#ifdef CONFIG_BEFS_CONV
			putname (node);
#endif
			putname (node);
			brelse (bh);
			continue;
		}

#ifdef CONFIG_BEFS_CONV
		putname (node);
#endif
		k = filp->f_pos - count;

		/*
		 * search key
		 */

		if (!befs_get_key_from_index_node (bn, &k, BEFS_TYPE(sb),
			tmpname, &len, &value)) {

			putname (tmpname);
			brelse (bh);

			return -EBADF;
		}

		/*
		 * Convert UTF-8 to nls charset
		 */

		if (!befs_utf2nls (tmpname, len, &tmpname2, &len_dist, sb)) {
			putname (tmpname);
			brelse (bh);

			return -ENOMEM;
		}
		error = filldir (dirent, tmpname2, len_dist, filp->f_pos,
			(ino_t) value);
		putname (tmpname2);

		if (error)
			filp->f_pos = 0;
		else
			filp->f_pos++;

		brelse (bh);

		goto finish;
	}

	filp->f_pos = 0;
finish:
	putname (tmpname);

	BEFS_OUTPUT (("<--- befs_readdir() filp->f_pos %d\n",
		filp->f_pos));
		
	return 0;
}
