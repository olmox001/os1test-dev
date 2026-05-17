/*
 * kernel/libkernel/include/libkernel/vfs/file.h
 * Per-open-file descriptor state (libkernel).
 */
#ifndef _LIBKERNEL_VFS_FILE_H
#define _LIBKERNEL_VFS_FILE_H

#include <libkernel/types.h>

struct vnode;

#define O_RDONLY  0x0000
#define O_WRONLY  0x0001
#define O_RDWR    0x0002
#define O_CREAT   0x0040
#define O_TRUNC   0x0200
#define O_APPEND  0x0400

struct file {
    struct vnode *f_vnode;
    int64_t       f_offset;
    int           f_flags;
    int           f_refcount;
};

#define MAX_FDS 16

struct file *file_alloc(struct vnode *vp, int flags);
void         file_free(struct file *fp);

#endif /* _LIBKERNEL_VFS_FILE_H */
