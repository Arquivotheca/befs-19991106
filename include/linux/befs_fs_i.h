/*
 *  linux/include/linux/befs_fs_i.h
 *
 * Copyright (C) 1999  Makoto Kato (m_kato@ga2.so-net.ne.jp)
 *
 *  from
 *
 *  linux/include/linux/ex2_fs_i.h
 */

#ifndef _LINUX_BEFS_FS_I
#define _LINUX_BEFS_FS_I

#include <linux/befs_fs.h>

struct befs_inode_info {
	befs_inode_addr	i_inode_num;
	__u32	i_uid;
	__u32	i_gid;
	__u32	i_mode;
	__u32	i_flags;

	befs_inode_addr	i_parent;
	befs_inode_addr	i_attribute;

	__u32	i_type;

	union {
		befs_data_stream ds;
		char            symlink[BEFS_SYMLINK_LEN];
	} i_data;

	void * i_index_cache;
};

#endif /* _LINUX_BEFS_FS_I */
