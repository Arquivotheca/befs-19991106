/*
 *  linux/fs/befs/file.c
 *
 * Copyright (C) 1999  Makoto Kato (m_kato@ga2.so-net.ne.jp)
 *
 *  from
 *
 *  linux/fs/minix/file.c
 *
 *  Copyright (C) 1991, 1992  Linus Torvalds
 */

#include <asm/uaccess.h>
#include <asm/system.h>

#include <linux/errno.h>
#include <linux/fs.h>
#include <linux/befs_fs.h>
#include <linux/fcntl.h>
#include <linux/sched.h>
#include <linux/stat.h>
#include <linux/locks.h>
#include <linux/mm.h>
#include <linux/pagemap.h>


ssize_t befs_file_read (struct file *, char *,  size_t, loff_t *);
static befs_inode_addr befs_startpos_from_ds (struct super_block *,
	befs_data_stream *, off_t);
static int befs_read_indirect_block (struct super_block *, const befs_inode_addr,
	const int, befs_inode_addr *);
static int befs_read_double_indirect_block (struct super_block *,
	const befs_inode_addr, const int, befs_inode_addr *);
static ssize_t befs_file_write (struct file *, const char *, size_t, loff_t *);


static struct file_operations befs_file_ops =
{
	NULL,				/* lseek */
	befs_file_read,			/* read */
	NULL,				/* write */
	NULL,				/* readdir - bad */
	NULL,				/* poll - default */
	NULL,				/* ioctl - default */
	generic_file_mmap,		/* mmap */
	NULL,
	NULL,				/* flush */
	NULL,				/* release */
	file_fsync,			/* fsync */
};

struct inode_operations befs_file_inode_operations =
{
	&befs_file_ops,			/* default file operations */
	NULL,				/* create */
	NULL,				/* lookup */
	NULL,				/* link */
	NULL,				/* unlink */
	NULL,				/* symlink */
	NULL,				/* mkdir */
	NULL,				/* rmdir */
	NULL,				/* mknod */
	NULL,				/* rename */
	NULL,				/* readlink */
	NULL,				/* follow_link */
	NULL,				/* get_block */
	block_read_full_page,		/* readpage */
	NULL,				/* writepage */
	NULL,				/* flushpage */
	NULL,				/* truncate */
	NULL,				/* permission */
	NULL,				/* smap */
	NULL				/* revalidate */
};


ssize_t befs_file_read(struct file *filp, char *buf,  size_t count, loff_t *ppos)
{
	struct inode *       inode = filp->f_dentry->d_inode;
	struct super_block * sb = inode->i_sb;
	befs_data_stream *    ds = &inode->u.befs_i.i_data.ds;
	befs_inode_addr       iaddr;
	befs_inode_addr       iaddr2;
	off_t                pos = *ppos;
	off_t                sum = 0;
	int                  i;
	int                  j;
	off_t                read_count;
	off_t                offset;

	BEFS_OUTPUT (("---> befs_file_read() "
		"inode %lu count %lu ppos %Lu\n",
		inode->i_ino, (__u32) count, *ppos));

	/*
	 * Get start position
	 */

	for (i = 0; ;) {
		iaddr = befs_read_data_stream (sb, ds, &i);
		if (BEFS_IS_EMPTY_IADDR(&iaddr)) {
			BEFS_OUTPUT ((" end data_stream\n"));
			break;
		}

		if ((pos >= sum)
			&& (pos < sum + iaddr.len * sb->u.befs_sb.block_size)) {

			iaddr.start += (pos - sum) / sb->u.befs_sb.block_size;
			iaddr.len -= (pos - sum) / sb->u.befs_sb.block_size;
			break;
		}
		sum += iaddr.len * sb->u.befs_sb.block_size;
	}

	read_count = 0;
	offset = (pos - sum) % sb->u.befs_sb.block_size;
	iaddr2 = iaddr;

	count = inode->i_size > pos + count ? count : inode->i_size - pos;

	for ( j = 0; j < iaddr.len; j++) {
		struct buffer_head * bh;

		bh = befs_bread2 (sb, iaddr2);
		if (!bh)
			return -EBADF;

		BEFS_OUTPUT ((" read_count %Ld offset %Ld\n",
			read_count, offset));

		if (count > sb->u.befs_sb.block_size) {
			memcpy (buf + read_count, bh->b_data + offset,
				sb->u.befs_sb.block_size - offset);

			*ppos += sb->u.befs_sb.block_size - offset;
			count -= sb->u.befs_sb.block_size - offset;
			read_count += sb->u.befs_sb.block_size - offset;
			offset = 0;

			++iaddr2.start;

			brelse (bh);
		} else {
			memcpy (buf + read_count, bh->b_data + offset, count);

			*ppos += count;
			read_count += count;
	
			brelse (bh);
			break;
		}
	}

	ppos += read_count;

	BEFS_OUTPUT (("<--- befs_file_read() "
		"return value %d, ppos %ld\n", read_count, *ppos));

	return read_count;
}

/*
 * Read indirect block
 */

static int befs_read_indirect_block (struct super_block * sb,
	const befs_inode_addr indirect, const int pos, 
	befs_inode_addr * iaddr)
{
	befs_inode_addr *     array;
	struct buffer_head * bh;
	befs_inode_addr       addr = indirect;
	int                  p = pos;

	BEFS_OUTPUT (("---> befs_read_indirect_block()\n"));

	/*
	 * explore nessealy block
	 */

	addr.start += p / BEFS_BLOCK_PER_INODE(sb);
	addr.len -= p / BEFS_BLOCK_PER_INODE(sb);

	if (addr.len < 1)
		return -EBADF;

	p = p % BEFS_BLOCK_PER_INODE(sb);

	bh = befs_bread2 (sb, addr);
	if (!bh)
		return -EBADF;

	array = (befs_inode_addr *) bh->b_data;

	/*
	 * Is this block inode address??
	 */

	if (!BEFS_IS_EMPTY_IADDR(&array[p % BEFS_BLOCK_PER_INODE(sb)])) {
		*iaddr = array[p % BEFS_BLOCK_PER_INODE(sb)];
	} else {
		brelse (bh);
		return -EBADF;
	}

	brelse (bh);

	BEFS_OUTPUT (("<--- befs_read_indirect_block()\n"));

	return 0;
}

/*
 * befs_read_double_indirect_block
 *
 * description:
 *  Read double-indirect block
 *
 * parameter:
 *  sb              ... super block
 *  double_indirect ... inode address of double-indirect block
 *  pos             ... position of double-indirect block
 *                       (*pos - max_direct_block - max_indirect_block)
 *
 * return value:
 *  0 ... sucess
 */

static int befs_read_double_indirect_block (struct super_block * sb,
        const befs_inode_addr double_indirect, const int pos,
	befs_inode_addr * iaddr)
{
        struct buffer_head *  bh_indirect;
        int                   p = pos;
        befs_inode_addr *      indirects;
	befs_inode_addr        addr = double_indirect;
	int                   i;

	BEFS_OUTPUT (("---> befs_read_double_indirect_block() \n"));

        while (addr.len) {
                bh_indirect = befs_bread2 (sb, addr);

                if (!bh_indirect) {
                        BEFS_OUTPUT (("cannot read double-indirect block "
                                "[%lu, %u, %u]\n",
                                double_indirect.allocation_group,
                                double_indirect.start, double_indirect.len));
                        return -EBADF;
                }

                addr.start++;
                addr.len--;

                indirects = (befs_inode_addr *) bh_indirect->b_data;

                for (i = 0; i < BEFS_BLOCK_PER_INODE(sb); i++) {
                        if (p < indirects[i].len * BEFS_BLOCK_PER_INODE(sb)) {

                                /*
                                 * find block!
                                 */

				int err;

				err = befs_read_indirect_block (sb,
					indirects[i], p, iaddr);
				brelse (bh_indirect);

				return err;
                        }

                        p -= indirects[i].len * BEFS_BLOCK_PER_INODE(sb);
                }

                brelse(bh_indirect);
        }

        /*
         * cannot find block...
         */

	return -EBADF;
}


befs_inode_addr befs_read_data_stream (struct super_block * sb,
	befs_data_stream * ds, int * pos)
{
        befs_inode_addr       iaddr = {0, 0, 0};

	BEFS_OUTPUT (("---> befs_read_data_stream() pos %d\n", *pos));

        if( *pos < 0 )
                return iaddr;

        if (*pos < ds->max_direct_range) {

		/*
		 * This position is in direct block.
		 */

		BEFS_OUTPUT ((" read in direct block [%lu, %u, %u]\n",
			ds->direct[*pos].allocation_group,
			ds->direct[*pos].start, ds->direct[*pos].len));

		/*
		 * Is this block inode address??
		 */

                if (!BEFS_IS_EMPTY_IADDR(&ds->direct[*pos]))
			iaddr = ds->direct[(*pos)++];
        } else if (*pos < ds->max_indirect_range + ds->max_direct_range) {

		/*
		 * This position is in in-direct block.
		 */

		int  p = *pos - ds->max_direct_range;
		
		BEFS_OUTPUT ((" read in indirect block [%lu, %u, %u]\n",
			ds->indirect.allocation_group, ds->indirect.start,
			ds->indirect.len));

		if (!befs_read_indirect_block (sb, ds->indirect, p, &iaddr))
			(*pos)++;
        } else if (*pos < ds->max_double_indirect_range
		+ ds->max_direct_range + ds->max_indirect_range) {

		/*
		 * This position is in double-in-direct block.
		 */

		int  p = *pos - ds->max_direct_range - ds->max_indirect_range;

		BEFS_OUTPUT ((" read in double indirect block\n"));

		if (!befs_read_double_indirect_block (sb, ds->double_indirect,
			p, &iaddr)) {

			(*pos)++;
		} 
        }

	if (BEFS_IS_EMPTY_IADDR(&iaddr))
		*pos = 0;

        return iaddr;
}

/*
 * Get inode address of start position form data stream
 */

static befs_inode_addr befs_startpos_from_ds (struct super_block * sb,
					befs_data_stream * ds, off_t pos)
{
	befs_inode_addr iaddr;
	off_t          sum = 0;
	int            i = 0;

	BEFS_OUTPUT (("---> befs_startpos_from_ds()\n"));

	/*
	 * Get start position
	 */

	while(1) {
		iaddr = befs_read_data_stream (sb, ds, &i);
		if (BEFS_IS_EMPTY_IADDR(&iaddr)) {
			BEFS_OUTPUT ((" end data_stream\n"));
			break;
		}

		if ((pos >= sum)
			&& (pos < sum + iaddr.len * sb->u.befs_sb.block_size)) {

			iaddr.start += (pos - sum) / sb->u.befs_sb.block_size;
			iaddr.len -= (pos - sum) / sb->u.befs_sb.block_size;
			break;
		}

		sum += iaddr.len * sb->u.befs_sb.block_size;
	}

	BEFS_OUTPUT (("<--- befs_startpos_from_ds()\n"));

	return iaddr;
}
