/*
 * linux/include/linux/befs_fs.h
 *
 * Copyright (C) 1999 Makoto Kato (m_kato@ga2.so-net.ne.jp)
 */

#ifndef _LINUX_BEFS_FS
#define _LINUX_BEFS_FS

#include <linux/types.h>

/*
 * for debug
 */
/* #define BEFS_DEBUG */

#ifdef BEFS_DEBUG
#define BEFS_OUTPUT(x)        printk x
#define BEFS_DUMP_SUPER_BLOCK befs_dump_super_block
#define BEFS_DUMP_INODE       befs_dump_inode
#define BEFS_DUMP_INDEX_ENTRY befs_dump_index_entry
#define BEFS_DUMP_INDEX_NODE  befs_dump_index_node
#define BEFS_DUMP_INODE_ADDR  befs_dump_inode_addr
#else
#define BEFS_OUTPUT(x)
#define BEFS_DUMP_SUPER_BLOCK(x)
#define BEFS_DUMP_INODE(x)
#define BEFS_DUMP_INDEX_ENTRY(x)
#define BEFS_DUMP_INDEX_NODE(x)
#define BEFS_DUMP_INODE_ADDR(x)
#endif


/*
 * Flags of superblock
 */

#define BEFS_CLEAN 0x434c454e
#define BEFS_DIRTY 0x44495254

/*
 * Max name length of BFS
 */

#define BEFS_NAME_LEN 255
#define BEFS_SYMLINK_LEN 160

/*
 * Magic headers of BFS's superblock, inode and index
 */

#define BEFS_SUPER_BLOCK_MAGIC1 0x42465331 /* BFS1 */
#define BEFS_SUPER_BLOCK_MAGIC2 0xdd121031
#define BEFS_SUPER_BLOCK_MAGIC3 0x15b6830e

#define BEFS_INODE_MAGIC1 0x3bbe0ad9

#define BEFS_INDEX_MAGIC 0x69f6c2e8

#define BEFS_SUPER_MAGIC BEFS_SUPER_BLOCK_MAGIC1


#define BEFS_NUM_DIRECT_BLOCKS 12
#define B_OS_NAME_LENGTH 32

/*
 * BeOS filesystem type
 */

#define BEFS_PPC 1
#define BEFS_X86 2

/*
 * default gid and uid
 */

#define BEFS_DEFAULT_GID 0
#define BEFS_DEFAULT_UID 0

/*
 * Flags of inode
 */

#define BEFS_INODE_IN_USE      0x00000001
#define BEFS_ATTR_INODE        0x00000004
#define BEFS_INODE_LOGGED      0x00000008
#define BEFS_INODE_DELETED     0x00000010

#define BEFS_PERMANENT_FLAG    0x0000ffff

#define BEFS_INODE_NO_CREATE   0x00010000
#define BEFS_INODE_WAS_WRITTEN 0x00020000
#define BEFS_NO_TRANSACTION    0x00040000


#define BEFS_IADDR2INO(iaddr,sb) \
	(((iaddr)->allocation_group << (sb)->ag_shift) + (iaddr)->start)
#define BEFS_INODE2INO(inode) \
	(((inode)->u.befs_i.i_inode_num.allocation_group << \
		(inode)->i_sb->u.befs_sb.ag_shift) \
	+ (inode)->u.befs_i.i_inode_num.start)

#define BEFS_BLOCK_PER_INODE(sb) \
	((sb)->u.befs_sb.block_size / sizeof(befs_inode_addr))

#define BEFS_IS_EMPTY_IADDR(iaddr) \
	((!(iaddr)->allocation_group) && (!(iaddr)->start) && (!(iaddr)->len))

#define BEFS_TYPE(sb) \
	((sb)->u.befs_sb.mount_opts.befs_type)

/* 
 * special type of BFS
 */

typedef __s64	befs_off_t;
typedef __s64	befs_bigtime_t;
typedef void	befs_binode_etc;


/* Block runs */
typedef struct _befs_block_run {
	__u32	allocation_group;
	__u16	start;
	__u16	len;
} __attribute__ ((packed)) befs_block_run;


typedef befs_block_run	befs_inode_addr;


/*
 * The Superblock Structure
 */

typedef struct _befs_super_block {
	char	name[B_OS_NAME_LENGTH];
	__u32	magic1;
	__u32	fs_byte_order;

	__u32	block_size;
	__u32	block_shift;

	befs_off_t  num_blocks;
	befs_off_t  used_blocks;

	__u32          inode_size;

	__u32          magic2;
	__u32          blocks_per_ag;
	__u32          ag_shift;
	__u32          num_ags;

	__u32		flags;

	befs_block_run  log_blocks;
	befs_off_t      log_start;
	befs_off_t      log_end;

	__u32		magic3;
	befs_inode_addr root_dir;
	befs_inode_addr indices;

	__u32          pad[8];
} __attribute__ ((packed)) befs_super_block;


typedef struct _befs_data_stream {
	befs_block_run	direct[BEFS_NUM_DIRECT_BLOCKS];
	befs_off_t	max_direct_range;
	befs_block_run	indirect;
	befs_off_t	max_indirect_range;
	befs_block_run	double_indirect;
	befs_off_t	max_double_indirect_range;
	befs_off_t	size;
} __attribute__ ((packed)) befs_data_stream;


/* Attribute */
typedef struct _befs_small_data {
	__u32	type;
	__u16	name_size;
	__u16	data_size;
} __attribute__ ((packed)) befs_small_data;


/* Inode structure */
typedef struct _befs_inode {
	__u32           magic1;
	befs_inode_addr  inode_num;
	__u32           uid;
	__u32           gid;
	__u32           mode;
	__u32           flags;
	befs_bigtime_t	create_time;
	befs_bigtime_t	last_modified_time;
	befs_inode_addr	parent;
	befs_inode_addr	attributes;
	__u32           type;

	__u32           inode_size;
	__u32           etc; /* not use */

	union {
		befs_data_stream	datastream;
		char			symlink[BEFS_SYMLINK_LEN];
	} data;
	__u32		pad[4]; /* not use */
} __attribute__ ((packed)) befs_inode;


typedef struct _befs_index_entry {
	__u32      magic;
	__u32      node_size;
	__u32      max_number_of_levels;
	__u32      data_type;
	befs_off_t  root_node_pointer;
	befs_off_t  free_node_pointer;
	befs_off_t  maximum_size;
} __attribute__ ((packed)) befs_index_entry;
	

typedef struct _befs_index_node {
	befs_off_t left;
	befs_off_t right;
	befs_off_t overflow;
	__u16     all_key_count;
	__u16     all_key_length;
} __attribute__ ((packed)) befs_index_node;


typedef struct _befs_mount_options {
	gid_t	gid;
	uid_t	uid;
	int	befs_type;
	char *  iocharset;
} befs_mount_options;


#ifdef __KERNEL__
/*
 * Function prototypes
 */

/* file.c */
extern int befs_read (struct inode *, struct file *, char *, int);
extern befs_inode_addr befs_read_data_stream (struct super_block *,
	befs_data_stream *, int *);

/* inode.c */
extern int befs_bmap (struct inode *, int);
extern void befs_read_inode (struct inode *);
extern struct buffer_head * befs_bread (struct inode *);
extern struct buffer_head * befs_bread2 (struct super_block *, befs_inode_addr);
extern void befs_write_inode (struct inode *);
extern int befs_sync_inode (struct inode *);

/* namei.c */
extern void befs_release (struct inode *, struct file *);
extern int befs_lookup (struct inode *, struct dentry *);
extern int befs_unlink (struct inode *, struct dentry *);
extern int befs_mkdir (struct inode *, struct dentry *, int);
extern int befs_rmdir (struct inode *, struct dentry *);
extern int befs_symlink (struct inode *, struct dentry *, const char *);

/* super.c */
extern void befs_put_super (struct super_block *);
extern int befs_remount (struct super_block *, int *, char *);
extern struct super_block * befs_read_super (struct super_block *,void *,int);
extern int init_befs_fs (void);
extern int befs_statfs (struct super_block *, struct statfs *, int);

/* index.c */
extern char * befs_get_key_from_index_node (befs_index_node *, int *, int,
	char *, int *, befs_off_t *);
extern struct buffer_head * befs_read_index_node (befs_inode_addr,
	struct super_block *, int, befs_off_t *);
extern void befs_convert_index_node (int, befs_index_node *, befs_index_node *);

/* debug.c */
extern void befs_dump_super_block (befs_super_block *);
extern void befs_dump_inode (befs_inode *);
extern void befs_dump_index_entry (befs_index_entry *);
extern void befs_dump_index_node (befs_index_node *);

/* util.c */
extern char * befs_utf2nls (char *, int, char **, int *, struct super_block *);
extern char * befs_nls2utf (char *, int, char **, int *, struct super_block *);

/*
 * Inodes and files operations
 */

/* dir.c */
extern struct inode_operations befs_dir_inode_operations;

/* file.c */
extern struct inode_operations befs_file_inode_operations;

/* symlink.c */
extern struct inode_operations befs_symlink_inode_operations;

#endif  /* __KERNEL__ */



#endif
