/*
 *  linux/fs/befs/inode.c
 *
 * Copyright (C) 1999  Makoto Kato (m_kato@ga2.so-net.ne.jp)
 *
 *  from
 *
 *  linux/fs/minix/inode.c
 *
 *  Copyright (C) 1991, 1992  Linus Torvalds
 */

#include <asm/uaccess.h>
#include <asm/system.h>

#include <linux/errno.h>
#include <linux/fs.h>
#include <linux/befs_fs.h>
#include <linux/befs_fs_sb.h>
#include <linux/befs_fs_i.h>
#include <linux/sched.h>
#include <linux/stat.h>
#include <linux/string.h>
#include <linux/locks.h>
#include <linux/mm.h>


static int befs_update_inode(struct inode *, int);

/*
 * return blocknumber
 */

int befs_bmap (struct inode * inode, int block)
{
	struct befs_sb_info    bs = inode->i_sb->u.befs_sb;
	struct befs_inode_info bi = inode->u.befs_i;

	BEFS_OUTPUT (("---> Enter befs_bmap\n"));

	BEFS_OUTPUT (("<--- Enter befs_bmap\n"));

	return  (bi.i_inode_num.allocation_group << bs.ag_shift)
		+ bi.i_inode_num.start;
}


/*
 * befs_bread function
 *
 *  get buffer_header for inode.
 */

struct buffer_head * befs_bread (struct inode * inode)
{
	befs_off_t             offset;
	struct super_block *  sb = inode->i_sb;
	dev_t                 dev = sb->s_dev;

	BEFS_OUTPUT (("---> Enter befs_read "
		"[%lu, %u, %u]\n",
		inode->u.befs_i.i_inode_num.allocation_group,
		inode->u.befs_i.i_inode_num.start,
		inode->u.befs_i.i_inode_num.len));

	offset = (inode->u.befs_i.i_inode_num.allocation_group
		<< sb->u.befs_sb.ag_shift) + inode->u.befs_i.i_inode_num.start;

	BEFS_OUTPUT (("<--- Enter befs_read\n"));

	return bread (dev, offset, sb->s_blocksize);
}


struct buffer_head * befs_bread2 (struct super_block * sb, befs_inode_addr inode)
{
	dev_t     dev = sb->s_dev;
	befs_off_t offset;

	BEFS_OUTPUT (("---> Enter befs_read2 "
		"[%lu, %u, %u]\n",
		inode.allocation_group, inode.start, inode.len));

	offset = (inode.allocation_group << sb->u.befs_sb.ag_shift)
		+ inode.start;
	
	BEFS_OUTPUT (("<--- Enter befs_read2\n"));

	return bread (dev, offset, sb->s_blocksize);
}


/*
 * Convert little-endian and big-endian
 */

void befs_convert_inodeaddr (int fstype, befs_inode_addr * iaddr,
	befs_inode_addr * out)
{
	switch (fstype)
	{
	case BEFS_X86:
		out->allocation_group = le32_to_cpu(iaddr->allocation_group);
		out->start = le16_to_cpu(iaddr->start);
		out->len = le16_to_cpu(iaddr->len);
		break;

	case BEFS_PPC:
		out->allocation_group = be32_to_cpu(iaddr->allocation_group);
		out->start = be16_to_cpu(iaddr->start);
		out->len = be16_to_cpu(iaddr->len);
		break;
	}
}


static void befs_convert_inode (int fstype, befs_inode *inode, befs_inode * out)
{
	int i;

	/*
	 * Convert data stream.
	 */

	switch (fstype)
	{
	case BEFS_X86:
		out->magic1 = le32_to_cpu(inode->magic1);
		befs_convert_inodeaddr (fstype, &inode->inode_num,
			&out->inode_num);
		out->uid = le32_to_cpu(inode->uid);
		out->gid = le32_to_cpu(inode->gid);
		out->mode = le32_to_cpu(inode->mode);
		out->flags = le32_to_cpu(inode->flags);
		out->create_time = le64_to_cpu(inode->create_time);
		out->last_modified_time =
			le64_to_cpu(inode->last_modified_time);
		inode->type = le32_to_cpu(inode->type);
		befs_convert_inodeaddr (fstype, &inode->parent, &out->parent);
		befs_convert_inodeaddr (fstype, &inode->attributes,
			&out->attributes);
		out->inode_size = le32_to_cpu(inode->inode_size);

		if (!S_ISLNK((umode_t) out->mode)) {
			/*
			 * Convert data stream.
			 */

			for ( i = 0; i < BEFS_NUM_DIRECT_BLOCKS; i++) {
				befs_convert_inodeaddr (fstype,
					&inode->data.datastream.direct[i],
					&out->data.datastream.direct[i]);
			}
			befs_convert_inodeaddr (fstype,
				&inode->data.datastream.indirect,
				&out->data.datastream.indirect);
			befs_convert_inodeaddr (fstype,
				&inode->data.datastream.double_indirect,
				&out->data.datastream.double_indirect);
	
			out->data.datastream.max_direct_range =
				le64_to_cpu(inode->data.datastream.max_direct_range);
			out->data.datastream.max_indirect_range =
				le64_to_cpu(inode->data.datastream.max_indirect_range);
			out->data.datastream.max_double_indirect_range =
				le64_to_cpu(inode->data.datastream.max_double_indirect_range);
			out->data.datastream.size =
				le64_to_cpu(inode->data.datastream.size);
		}
		break;

	case BEFS_PPC:
		out->magic1 = be32_to_cpu(inode->magic1);
		befs_convert_inodeaddr (fstype, &inode->inode_num,
			&out->inode_num);
		out->uid = be32_to_cpu(inode->uid);
		out->gid = be32_to_cpu(inode->gid);
		out->mode = be32_to_cpu(inode->mode);
		out->flags = be32_to_cpu(inode->flags);
		out->create_time = be64_to_cpu(inode->create_time);
		out->last_modified_time =
			be64_to_cpu(inode->last_modified_time);
		out->type = be32_to_cpu(inode->type);
		befs_convert_inodeaddr (fstype, &inode->parent, &out->parent);
		befs_convert_inodeaddr (fstype, &inode->attributes,
			&out->attributes);
		out->inode_size = be32_to_cpu(inode->inode_size);

		if (!S_ISLNK((umode_t) out->mode)) {

			/*
			 * Convert data stream.
			 */

			for ( i = 0; i < BEFS_NUM_DIRECT_BLOCKS; i++) {
				befs_convert_inodeaddr (fstype,
					&inode->data.datastream.direct[i],
					&out->data.datastream.direct[i]);
			}
			befs_convert_inodeaddr (fstype,
				&inode->data.datastream.indirect,
				&out->data.datastream.indirect);
			befs_convert_inodeaddr (fstype,
				&inode->data.datastream.double_indirect,
				&out->data.datastream.double_indirect);
	
			out->data.datastream.max_direct_range =
				be64_to_cpu(inode->data.datastream.max_direct_range);
			out->data.datastream.max_indirect_range =
				be64_to_cpu(inode->data.datastream.max_indirect_range);
			out->data.datastream.max_double_indirect_range =
				be64_to_cpu(inode->data.datastream.max_double_indirect_range);
			out->data.datastream.size =
				be64_to_cpu(inode->data.datastream.size);
		}
		break;
	}
}


void befs_read_inode (struct inode * inode)
{
	struct buffer_head * bh = NULL;
	befs_inode *          raw_inode = NULL;
	befs_inode *          disk_inode;

	BEFS_OUTPUT (("---> befs_read_inode() "
		"inode = %lu[%lu, %u, %u]\n",
		inode->i_ino,
		inode->u.befs_i.i_inode_num.allocation_group,
		inode->u.befs_i.i_inode_num.start,
		inode->u.befs_i.i_inode_num.len));

	/*
	 * convert from vfs's inode number to befs's inode number
	 */

	if (inode->i_ino != BEFS_INODE2INO(inode))
	{
		inode->u.befs_i.i_inode_num.allocation_group =
			inode->i_ino >> inode->i_sb->u.befs_sb.ag_shift;
		inode->u.befs_i.i_inode_num.start = inode->i_ino
				& ((2 << inode->i_sb->u.befs_sb.ag_shift) - 1);
		inode->u.befs_i.i_inode_num.len = 1; /* dummy */

		BEFS_OUTPUT (("  real inode number [%lu, %u, %u]\n",
			inode->u.befs_i.i_inode_num.allocation_group,
			inode->u.befs_i.i_inode_num.start,
			inode->u.befs_i.i_inode_num.len));
	}

	bh = befs_bread (inode);
	if (!bh) {
		printk (KERN_ERR "BEFS: unable to read inode block - "
			"inode = %lu", inode->i_ino);
		goto bad_inode;
	}

	disk_inode = (befs_inode *) bh->b_data;

	/*
	 * Convert little-endian or big-endian from befs type.
	 */
#ifdef CONFIG_BEFS_CONV
	raw_inode = (befs_inode *) __getname();
	if (!raw_inode) {
		printk (KERN_ERR "BEFS: not allocate memory - inode = %lu",
			inode->i_ino);
		goto bad_inode;
	}
	befs_convert_inode (BEFS_TYPE(inode->i_sb), disk_inode, raw_inode);
#else
	raw_inode = disk_inode;
#endif

	BEFS_DUMP_INODE (raw_inode);

	/*
	 * check magic header.
	 */
	if (raw_inode->magic1 != BEFS_INODE_MAGIC1) {
		printk (KERN_ERR
			"BEFS: this inode is bad magic header - inode = %lu\n",
			inode->i_ino);
		goto bad_inode;
	}

	/*
	 * check flag
	 */

	if (!(raw_inode->flags & BEFS_INODE_IN_USE) ||
		(raw_inode->flags & BEFS_INODE_DELETED)) {

		printk (KERN_ERR "BEFS: inode is not used - inode = %lu\n",
			inode->i_ino);
		goto bad_inode;
	}

	inode->i_mode = (umode_t) raw_inode->mode;

	/*
	 * set uid and gid.  But since current BeOS is single user OS, so
	 * you can change by "uid" or "gid" options.
	 */

	inode->i_uid = inode->i_sb->u.befs_sb.mount_opts.uid ?
		inode->i_sb->u.befs_sb.mount_opts.uid : (uid_t) raw_inode->uid;
	inode->i_gid = inode->i_sb->u.befs_sb.mount_opts.gid ?
		inode->i_sb->u.befs_sb.mount_opts.gid : (gid_t) raw_inode->gid;

	inode->i_nlink = 1;

	/*
	 * BEFS's time is 64 bits, but current VFS is 32 bits...
	 */

	inode->i_ctime = (time_t) (raw_inode->create_time >> 16);
	inode->i_mtime = (time_t) (raw_inode->last_modified_time >> 16);

	/*
	 * BEFS don't have access time.  So use last modified time.
	 */

	inode->i_atime = (time_t) (raw_inode->last_modified_time >> 16);

	inode->i_blksize = raw_inode->inode_size;
	inode->i_blocks = raw_inode->inode_size / inode->i_sb->s_blocksize;
	inode->i_version = ++event;

	inode->u.befs_i.i_inode_num = (befs_inode_addr) raw_inode->inode_num;
	inode->u.befs_i.i_mode = raw_inode->mode;
	inode->u.befs_i.i_flags = raw_inode->flags;
	inode->u.befs_i.i_parent = (befs_inode_addr) raw_inode->parent;
	inode->u.befs_i.i_attribute = (befs_inode_addr) raw_inode->attributes;

	/*
	 * Symbolic link have no data stream.
	 * This position is original filename.
	 */

	if (!S_ISLNK(inode->i_mode)) {
		inode->i_size = raw_inode->data.datastream.size;
		inode->u.befs_i.i_data.ds = raw_inode->data.datastream;
	} else {
		inode->i_size = 0;
		memcpy (inode->u.befs_i.i_data.symlink, disk_inode->data.symlink,
			BEFS_SYMLINK_LEN);
	}

	if (S_ISREG(inode->i_mode))
		inode->i_op = &befs_file_inode_operations;
	else if (S_ISDIR(inode->i_mode))
		inode->i_op = &befs_dir_inode_operations;
	else if (S_ISLNK(inode->i_mode))
		inode->i_op = &befs_symlink_inode_operations;
	else if (S_ISCHR(inode->i_mode))
		inode->i_op = &chrdev_inode_operations;
	else if (S_ISBLK(inode->i_mode))
		inode->i_op = &blkdev_inode_operations;
	else if (S_ISFIFO(inode->i_mode))
		init_fifo(inode);

#ifdef CONFIG_BEFS_CONV
	putname (raw_inode);
#endif
	brelse(bh);

	return;

bad_inode:
	make_bad_inode(inode);
#ifdef CONFIG_BEFS_CONV
	if (raw_inode)
		putname (raw_inode);
#endif
	if (bh)
		brelse(bh);

	return;
}

#ifdef CONFIG_BEFS_RW
static int befs_update_inode(struct inode * inode, int do_sync)
{
	struct buffer_head *  bh;
	befs_inode *           raw_inode;
	int                   err = 0;

	bh = befs_bread (inode);
	if (!bh) {
		printk(KERN_ERR "BEFS: "
			"bad inode number on dev %s: %ld is out of range\n",
			bdevname(inode->i_dev), inode->i_ino);
		return -EIO;
	}

	raw_inode = (befs_inode *) bh->b_data;

	/*
	 * fill raw_inode
	 */

	raw_inode->magic1 = BEFS_INODE_MAGIC1;
	raw_inode->inode_num = inode->u.befs_i.i_inode_num

	raw_inode->uid = BEFS_DEFAULT_UID;
	raw_inode->gid = BEFS_DEFAULT_GID;

	raw_inode->mode = inode->i_mode;
	raw_inode->flags = inode->u.befs_i.i_flags;

	raw_inode->create_time = ((__u64) inode->i_ctime) << 16;
	raw_inode->last_modified_time = ((__u64) inode->i_mtime) << 16;

	raw_inode->parent = inode->u.befs_i.i_parent;
	raw_inode->attributes = inode->u.befs_i.i_attribute;

	raw_inode->type = inode->u.befs_i.i_type;

	raw_inode->inode_size = inode->i_blksize;

	if (!S_ISLNK(inode->i_mode))
		raw_inode->data.datastream = inode->u.befs_i.i_data.ds;
	else
		memcpy (raw_inode->data.symlink, inode->u.befs_i.i_data.symlink,
			BEFS_SYMLINK_LEN);

	mark_buffer_dirty (bh, 1);

	if (do_sync) {
		ll_rw_block (WRITE, 1, &bh);
		wait_on_buffer (bh);
		if (buffer_req (bh) && !buffer_uptodate (bh)) {
			printk (KERN_ERR "BEFS: "
				"IO error syncing inode [%s: %08lx]\n",
				bdevname(inode->i_dev), inode->i_ino);
			err = -EIO;
		}
	}
	brelse (bh);

	return err;
}


void befs_write_inode (struct inode * inode)
{
	befs_update_inode (inode, 0);
}

int befs_sync_inode (struct inode *inode)
{
	return befs_update_inode (inode, 1);
}
#endif
