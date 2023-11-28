/*
 * linux/fs/befs/super.c
 *
 * Copyright (C) 1999  Makoto Kato (m_kato@ga2.so-net.ne.jp)
 *
 *  from
 *
 *  linux/fs/minix/inode.c
 *
 *  Copyright (C) 1991, 1992  Linus Torvalds
 */

#include <linux/module.h>

#include <stdarg.h>

#include <asm/bitops.h>
#include <asm/uaccess.h>
#include <asm/system.h>

#include <linux/errno.h>
#include <linux/fs.h>
#include <linux/befs_fs.h>
#include <linux/malloc.h>
#include <linux/sched.h>
#include <linux/stat.h>
#include <linux/string.h>
#include <linux/locks.h>
#include <linux/blkdev.h>
#include <linux/init.h>
#include <linux/nls.h>


void befs_put_super (struct super_block * sb)
{
	if (sb->u.befs_sb.mount_opts.iocharset) {
		kfree (sb->u.befs_sb.mount_opts.iocharset);
		sb->u.befs_sb.mount_opts.iocharset = NULL;
	}

	if (sb->u.befs_sb.nls) {
		unload_nls (sb->u.befs_sb.nls);
		sb->u.befs_sb.nls = NULL;
	}

	MOD_DEC_USE_COUNT;
	return;
}


static const struct super_operations befs_sops =
{
	befs_read_inode,		/* read_inode */
#ifdef CONFIG_BEFS_FS_RW
	befs_write_inode,		/* write_inode */
	NULL,				/* put_inode */
	NULL,				/* delete_inode */
#else
	NULL,				/* write_inode */
	NULL,				/* put_inode */
	NULL,				/* delete_inode */
#endif
	NULL,				/* notify_change */
	befs_put_super,			/* put_super */
#ifdef CONFIG_BEFS_FS_RW
	NULL,				/* write_super */
#else
	NULL,				/* write_super */
#endif
	befs_statfs,			/* statfs */
	befs_remount			/* remount_fs */
};


static int parse_options (char *, befs_mount_options *);
static void befs_convert_super_block (int, befs_super_block *,
	befs_super_block *);

static int parse_options (char *options, befs_mount_options * opts)
{
	char * this_char;
	char * value;
	int    ret = 1;

	/*
	 * Initialize option.
	 */
	opts->uid = 0;
	opts->gid = 0;
#ifdef CONFIG_PPC
	opts->befs_type = BEFS_PPC;
#else
	opts->befs_type = BEFS_X86;
#endif

	if (!options)
		return ret;

	for (this_char = strtok (options, ","); this_char != NULL;
		this_char = strtok (NULL, ",")) {

		if ((value = strchr (this_char, '=')) != NULL)
			*value++ = 0;

		if (!strcmp (this_char, "uid")) {
			if (!value || !*value) {
				ret = 0;
			} else {
				opts->uid = simple_strtoul (value, &value, 0);
				if (*value) {
					printk (KERN_ERR "BEFS: Invalid uid "
						"option: %s\n", value);
					ret = 0;
				}
			}
		} else if (!strcmp (this_char, "gid")) {
			if (!value || !*value)
				ret = 0;
			else {
				opts->gid = simple_strtoul(value, &value, 0);
				if (*value) {
					printk (KERN_ERR
						"BEFS: Invalid gid option: "
						"%s\n", value);
					ret = 0;
				}
			}
#ifdef CONFIG_BEFS_CONV
		} else if (!strcmp (this_char, "type")) {
			if (!value || !*value)
				return 0;
			else {
				if (!strcmp (value, "x86")) {
					opts->befs_type = BEFS_X86;
				} else if (!strcmp (value, "ppc")) {
					opts->befs_type = BEFS_PPC;
				} else {
					printk (KERN_ERR
						"BEFS: unknown filesystem"
						" type: %s\n", value);
					ret = 0;
				}
			}
#endif
		} else if (!strcmp (this_char,"iocharset") && value) {
			char * p = value;
			int    len;

			while (*value && *value != ',')
				value++;
			len = value - p;
			if (len) { 
				char * buffer = kmalloc (len+1, GFP_KERNEL);
				if (buffer) {
					opts->iocharset = buffer;
					memcpy (buffer, p, len);
					buffer[len] = 0;

					printk (KERN_INFO
						"BEFS: IO charset %s\n", buffer);
				} else {
					printk (KERN_ERR "BEFS: "
						"cannot allocate memory\n");
					ret = 0;
				}
			}
		}
	}

	return ret;
}


/*
 * Convert big-endian <-> little-endian
 */

static void befs_convert_super_block (int fstype, befs_super_block * bs,
	befs_super_block * out)
{
	switch (fstype)
	{
	case BEFS_X86:
		out->magic1 = le32_to_cpu(bs->magic1);
		out->block_size = le32_to_cpu(bs->block_size);
		out->block_shift = le32_to_cpu(bs->block_shift);
		out->num_blocks = le64_to_cpu(bs->num_blocks);
		out->used_blocks = le64_to_cpu(bs->used_blocks);
		out->inode_size = le32_to_cpu(bs->inode_size);
		out->magic2 = le32_to_cpu(bs->magic2);
		out->blocks_per_ag = le32_to_cpu(bs->blocks_per_ag);
		out->ag_shift = le32_to_cpu(bs->ag_shift);
		out->num_ags = le32_to_cpu(bs->num_ags);
		out->flags = le32_to_cpu(bs->flags);
		befs_convert_inodeaddr (fstype, &(bs->log_blocks),
			&(out->log_blocks));
		out->log_start = le64_to_cpu(bs->log_start);
		out->log_end = le64_to_cpu(bs->log_end);
		out->magic3 = le32_to_cpu(bs->magic3);
		befs_convert_inodeaddr (fstype, &(bs->root_dir),
			&(out->root_dir));
		befs_convert_inodeaddr (fstype, &(bs->indices),
			&(out->indices));
		break;

	case BEFS_PPC:
		out->magic1 = be32_to_cpu(bs->magic1);
		out->block_size = be32_to_cpu(bs->block_size);
		out->block_shift = be32_to_cpu(bs->block_shift);
		out->num_blocks = be64_to_cpu(bs->num_blocks);
		out->used_blocks = be64_to_cpu(bs->used_blocks);
		out->inode_size = be32_to_cpu(bs->inode_size);
		out->magic2 = be32_to_cpu(bs->magic2);
		out->blocks_per_ag = be32_to_cpu(bs->blocks_per_ag);
		out->ag_shift = be32_to_cpu(bs->ag_shift);
		out->num_ags = be32_to_cpu(bs->num_ags);
		out->flags = be32_to_cpu(bs->flags);
		befs_convert_inodeaddr (fstype, &(bs->log_blocks),
			&(out->log_blocks));
		out->log_start = be64_to_cpu(bs->log_start);
		out->log_end = be64_to_cpu(bs->log_end);
		out->magic3 = be32_to_cpu(bs->magic3);
		befs_convert_inodeaddr (fstype, &(bs->root_dir),
			&(out->root_dir));
		befs_convert_inodeaddr (fstype, &(bs->indices),
			&(out->indices));
		break;
	}
}


struct super_block * befs_read_super (struct super_block *sb, void *data,
	int silent )
{
	befs_super_block *    bs = NULL;
	kdev_t               dev = sb->s_dev;
	int                  blocksize;
	unsigned long        logic_sb_block;
	struct buffer_head * bh;

	BEFS_OUTPUT (("---> befs_read_super()\n"));

	/*
	 * Parse options
	 */

	if (!parse_options ((char*) data, &sb->u.befs_sb.mount_opts)) {
		printk (KERN_ERR "BEFS: cannot parse mount options\n");
		return NULL;
	}

	/*
	 * Set dummy blocksize to read super block.
	 */

	switch (BEFS_TYPE(sb))
	{
	case BEFS_X86:
		blocksize = 512;
		logic_sb_block = 1;
		break;

	case BEFS_PPC:
		blocksize = 1024;
		logic_sb_block = 0;
		break;

	default:
		printk (KERN_ERR "BEFS: cannot parse filesystem type\n");
		return NULL;	
	}

	MOD_INC_USE_COUNT;
	lock_super (sb);
	set_blocksize (dev, blocksize);

	/*
	 * Get BEFS super block.
	 */

	if (!(bh = bread (dev, logic_sb_block, blocksize))) {
		printk (KERN_ERR "BEFS: unable to read superblock\n");
		goto bad_befs_read_super;
	}

	bs = (befs_super_block *) bh->b_data;

#ifdef CONFIG_BEFS_CONV
	/*
	 * Convert byte order
	 */

	bs = (befs_super_block *) __getname();
	if (!bs) {
		brelse(bh);
		goto bad_befs_read_super;
	}
	befs_convert_super_block (BEFS_TYPE(sb),
		(befs_super_block *) bh->b_data, bs);	
#endif
	BEFS_DUMP_SUPER_BLOCK (bs);

	/*
	 * Check magic headers of super block
	 */

	if ((bs->magic1 != BEFS_SUPER_BLOCK_MAGIC1)
		|| (bs->magic2 != BEFS_SUPER_BLOCK_MAGIC2)
		|| (bs->magic3 != BEFS_SUPER_BLOCK_MAGIC3)) {

		brelse (bh);
		printk (KERN_ERR "BEFS: different magic header\n");
		goto bad_befs_read_super;
	}

	/*
	 * Check blocksize of BEFS.
	 *
	 * Blocksize of BEFS is 1024, 2048, 4096 or 8192.
	 */

	if( (bs->block_size != 1024)
		&& (bs->block_size != 2048)
		&& (bs->block_size != 4096)
		&& (bs->block_size != 8192)) {

		brelse (bh);
		printk (KERN_ERR "BEFS: different blocksize\n");
		goto bad_befs_read_super;
	}

	set_blocksize (dev, (int) bs->block_size);

	/*
	 * fill in standard stuff
	 */

	sb->s_magic = BEFS_SUPER_MAGIC;
	sb->s_blocksize = (int) bs->block_size;

	sb->u.befs_sb.block_size = bs->block_size;
	sb->u.befs_sb.block_shift = bs->block_shift;
	sb->u.befs_sb.num_blocks = bs->num_blocks;
	sb->u.befs_sb.used_blocks = bs->used_blocks;
	sb->u.befs_sb.inode_size = bs->inode_size;

	sb->u.befs_sb.blocks_per_ag = bs->blocks_per_ag;
	sb->u.befs_sb.ag_shift = bs->ag_shift;
	sb->u.befs_sb.num_ags = bs->num_ags;

	sb->u.befs_sb.log_blocks = bs->log_blocks;
	sb->u.befs_sb.log_start = bs->log_start;
	sb->u.befs_sb.log_end = bs->log_end;

	sb->u.befs_sb.root_dir = bs->root_dir;
	sb->u.befs_sb.indices = bs->indices;

	unlock_super (sb);

	/*
	 * set up enough so that it can read an inode
	 */

	sb->s_dev = dev;
	sb->s_op = (struct super_operations *) &befs_sops;
	sb->s_root = d_alloc_root (iget (sb,
		BEFS_IADDR2INO(&(bs->root_dir),bs)));

	if (!sb->s_root) {
		sb->s_dev = 0;
		brelse (bh);
		printk (KERN_ERR "BEFS: get root inode failed\n");

		goto uninit_last_befs_read_super;
	}

	/*
	 * load nls library
	 */

	if (sb->u.befs_sb.mount_opts.iocharset) {
		sb->u.befs_sb.nls =
			load_nls (sb->u.befs_sb.mount_opts.iocharset);
		if (!sb->u.befs_sb.nls) {
			printk (KERN_WARNING "BEFS: cannot load nls. "
				"loding default nls\n");
			sb->u.befs_sb.nls = load_nls_default ();
		}
	} else {
	}

	brelse (bh);
#ifdef CONFIG_BEFS_CONV
	putname (bs);
#endif

	return sb;

bad_befs_read_super:
	sb->s_dev = 0;
	unlock_super (sb);

uninit_last_befs_read_super:
#ifdef CONFIG_BEFS_CONV
	if (bs)
		putname (bs);
#endif
	MOD_DEC_USE_COUNT;

	return NULL;
}


int befs_remount (struct super_block * sb, int * flags, char * data)
{	
	if (!(*flags & MS_RDONLY))
		return -EINVAL;
	return 0;
}


static struct file_system_type befs_fs_type = {
	"befs", 
	FS_REQUIRES_DEV,
	befs_read_super, 
	NULL
};


int __init init_befs_fs(void)
{
	return register_filesystem(&befs_fs_type);
}

#ifdef MODULE
EXPORT_NO_SYMBOLS;

int init_module(void)
{
	return init_befs_fs();
}


void cleanup_module(void)
{
	unregister_filesystem(&befs_fs_type);
}

#endif


int befs_statfs (struct super_block * sb, struct statfs * buf, int bufsiz)
{
	struct statfs tmp;

	BEFS_OUTPUT (("---> befs_statfs()\n"));

	tmp.f_type = BEFS_SUPER_MAGIC;
	tmp.f_bsize = sb->s_blocksize;
	tmp.f_blocks = sb->u.befs_sb.num_blocks;
	tmp.f_bfree = sb->u.befs_sb.num_blocks - sb->u.befs_sb.used_blocks;
	tmp.f_bavail = sb->u.befs_sb.used_blocks;
	tmp.f_files = 0; /* UNKNOWN */
	tmp.f_ffree = 0; /* UNKNOWN */
	tmp.f_namelen = BEFS_NAME_LEN;
	
	return copy_to_user (buf, &tmp, bufsiz) ? -EFAULT : 0;
}
