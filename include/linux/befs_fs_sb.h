/*
 *  linux/include/linux/befs_fs_sb.h
 *
 * Copyright (C) 1999  Makoto Kato (m_kato@ga2.so-net.ne.jp)
 */

#ifndef _LINUX_BEFS_FS_SB
#define _LINUX_BEFS_FS_SB

#include <linux/befs_fs.h>

struct befs_sb_info {
	__u32		block_size;
	__u32		block_shift;
	befs_off_t	num_blocks;
	befs_off_t	used_blocks;
	__u32		inode_size;

	/*
	 * Allocation group information
	 */

	__u32	blocks_per_ag;
	__u32	ag_shift;
	__u32	num_ags;

	/*
	 * jornal log entry
	 */

	befs_block_run log_blocks;
	befs_off_t log_start;
	befs_off_t log_end;

	befs_inode_addr root_dir;
	befs_inode_addr indices;

	befs_mount_options mount_opts;

	struct nls_table * nls;
};
#endif
