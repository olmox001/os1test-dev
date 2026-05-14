/*
 * kernel/include/kernel/vfs_bsd.h
 * Backward-compatibility shim: pull in the new modular VFS headers.
 * Code that previously included this header continues to compile unchanged.
 */
#ifndef _KERNEL_VFS_BSD_H
#define _KERNEL_VFS_BSD_H

#include <kernel/vfs/vnode.h>
#include <kernel/vfs/mount.h>
#include <kernel/vfs/namei.h>

#endif /* _KERNEL_VFS_BSD_H */
