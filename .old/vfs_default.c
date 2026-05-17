/*
 * kernel/fs/vfs/vfs_default.c
 * Default VOP implementations returning EOPNOTSUPP
 */
#include <kernel/vfs/vnode.h>
#include <kernel/types.h>

static int vop_noop_open(struct vnode *vp __unused, int mode __unused, uid_t uid __unused) { return 0; }
static int vop_noop_close(struct vnode *vp __unused, int mode __unused, uid_t uid __unused) { return 0; }
static int vop_noop_reclaim(struct vnode *vp __unused) { return 0; }

static int vop_err_lookup(struct vnode *dvp __unused, struct vnode **vpp __unused,
                          struct componentname *cnp __unused) { return -ENOSYS; }
static int vop_err_read(struct vnode *vp __unused, void *buf __unused,
                        size_t len __unused, off_t *off __unused) { return -ENOSYS; }
static int vop_err_write(struct vnode *vp __unused, const void *buf __unused,
                         size_t len __unused, off_t *off __unused) { return -ENOSYS; }
static int vop_err_getattr(struct vnode *vp __unused, struct vattr *vap __unused) { return -ENOSYS; }
static int vop_err_setattr(struct vnode *vp __unused, struct vattr *vap __unused) { return -ENOSYS; }
static int vop_err_readdir(struct vnode *vp __unused, void *buf __unused,
                           size_t len __unused, off_t *off __unused) { return -ENOSYS; }
static int vop_err_access(struct vnode *vp __unused, mode_t mode __unused,
                          uid_t uid __unused) { return 0; }
static int vop_err_create(struct vnode *dvp __unused, struct vnode **vpp __unused,
                          struct componentname *cnp __unused,
                          struct vattr *vap __unused) { return -ENOSYS; }

struct vnode_ops vfs_default_vnodeops = {
    .vop_lookup  = vop_err_lookup,
    .vop_create  = vop_err_create,
    .vop_open    = vop_noop_open,
    .vop_close   = vop_noop_close,
    .vop_read    = vop_err_read,
    .vop_write   = vop_err_write,
    .vop_getattr = vop_err_getattr,
    .vop_setattr = vop_err_setattr,
    .vop_readdir = vop_err_readdir,
    .vop_reclaim = vop_noop_reclaim,
    .vop_access  = vop_err_access,
};
