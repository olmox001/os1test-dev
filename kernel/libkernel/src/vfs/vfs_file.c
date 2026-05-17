/*
 * kernel/libkernel/src/vfs/vfs_file.c
 * Per-fd file structure allocation (libkernel).
 */
#include <libkernel/vfs/vnode.h>
#include <libkernel/vfs/file.h>
#include <core/kmalloc.h>
#include <core/string.h>

struct file *file_alloc(struct vnode *vp, int flags) {
    struct file *fp = kmalloc(sizeof(struct file));
    if (!fp) return NULL;

    memset(fp, 0, sizeof(struct file));
    fp->f_vnode    = vp;
    fp->f_flags    = flags;
    fp->f_offset   = 0;
    fp->f_refcount = 1;
    vref(vp);
    return fp;
}

void file_free(struct file *fp) {
    if (!fp) return;
    kfree(fp);
}
