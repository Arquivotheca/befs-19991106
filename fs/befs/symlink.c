/*
 *  linux/fs/befs/symlink.c
 *
 * Copyright (C) 1999  Makoto Kato (m_kato@ga2.so-net.ne.jp)
 *
 *  from
 *
 *  linux/fs/minix/symlink.c
 *
 *  Copyright (C) 1991, 1992  Linus Torvalds
 */

#include <asm/uaccess.h>

#include <linux/errno.h>
#include <linux/fs.h>
#include <linux/befs_fs.h>
#include <linux/sched.h>
#include <linux/mm.h>
#include <linux/stat.h>

static int befs_readlink (struct dentry *, char *, int);
static struct dentry * befs_follow_link(struct dentry *, struct dentry *,
	unsigned int);

/*
 * symlinks can't do much...
 */
struct inode_operations befs_symlink_inode_operations = {
        NULL,                   /* no file-operations */
        NULL,                   /* create */
        NULL,                   /* lookup */
        NULL,                   /* link */
        NULL,                   /* unlink */
        NULL,                   /* symlink */
        NULL,                   /* mkdir */
        NULL,                   /* rmdir */
        NULL,                   /* mknod */
	NULL,			/* rename */
	befs_readlink,		/* readlink */
        befs_follow_link,       /* follow_link */
        NULL,                   /* readpage */
        NULL,                   /* writepage */
        NULL,                   /* bmap */
        NULL,                   /* truncate */
        NULL,                   /* permission */
        NULL                    /* smap */
};


/*
 * The inode of symbolic link is different to data stream.
 * The data stream become link name.
 */
static struct dentry * befs_follow_link(struct dentry * dentry,
	struct dentry * base, unsigned int follow)
{
        struct inode * inode = dentry->d_inode;
        char *         link = inode->u.befs_i.i_data.symlink;

        UPDATE_ATIME(inode);

        base = lookup_dentry (link, base, follow);

        return base;
}


static int befs_readlink (struct dentry * dentry, char * buffer, int buflen)
{
        struct inode * inode = dentry->d_inode;
        char *         link = inode->u.befs_i.i_data.symlink;
        int            len = 0;
        int            len_dist;
	char *         tmpname;

        if (buflen > BEFS_SYMLINK_LEN)
                buflen = BEFS_SYMLINK_LEN;

        while (len < buflen && link[len])
                len++;

	if (!befs_utf2nls (link, len, &tmpname, &len_dist, inode->i_sb))
		return -ENOMEM;

        if (copy_to_user(buffer, tmpname, len_dist))
                len = -EFAULT;

	putname (tmpname);

        UPDATE_ATIME(inode);

        return len;
}

