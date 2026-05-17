/*
 * kernel/core/include/core/vfs.h
 * VFS aggregation header — canonical definitions live in libkernel/vfs/.
 * Include this header to get the full VFS interface.
 */
#ifndef _KERNEL_VFS_H
#define _KERNEL_VFS_H

#include <libkernel/vfs/vnode.h>
#include <libkernel/vfs/mount.h>
#include <libkernel/vfs/namei.h>
#include <libkernel/vfs/file.h>

/* Ext4 VFS adaptor (kernel/core/src/fs/vfs_ext4.c) */
void ext4_vfs_init(void);
int  ext4_open_path(const char *path, struct vnode **vpp);

#endif /* _KERNEL_VFS_H */
