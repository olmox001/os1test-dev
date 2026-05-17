/*
 * kernel/libkernel/include/libkernel/vfs/vnode.h
 * BSD-style vnode interface — generic VFS layer (libkernel).
 * Architecture-independent; included by core and drivers alike.
 */
#ifndef _LIBKERNEL_VFS_VNODE_H
#define _LIBKERNEL_VFS_VNODE_H

#include <libkernel/types.h>
#include <libkernel/list.h>
#include <core/spinlock.h>

/* Vnode types */
typedef enum vtype {
    VNON, VREG, VDIR, VBLK, VCHR, VLNK, VSOCK, VFIFO, VBAD, VMARKER
} vtype_t;

/* Vnode flags */
#define VROOT   0x0001
#define VTEXT   0x0002
#define VSYSTEM 0x0004
#define VDOOMED 0x0008

/* File attributes (stat-like) */
struct vattr {
    vtype_t  va_type;
    uint32_t va_mode;
    uint32_t va_nlink;
    uint32_t va_uid;
    uint32_t va_gid;
    uint64_t va_fsid;
    uint64_t va_fileid;
    uint64_t va_size;
    int64_t  va_atime;
    int64_t  va_mtime;
    int64_t  va_ctime;
    uint32_t va_blocksize;
    uint64_t va_bytes;
};

struct vnode;
struct componentname;
struct mount;

/* Per-vnode operation table */
struct vnode_ops {
    int (*vop_lookup)(struct vnode *dvp, struct vnode **vpp,
                      struct componentname *cnp);
    int (*vop_create)(struct vnode *dvp, struct vnode **vpp,
                      struct componentname *cnp, struct vattr *vap);
    int (*vop_open)   (struct vnode *vp, int mode, uint32_t uid);
    int (*vop_close)  (struct vnode *vp, int mode, uint32_t uid);
    int (*vop_read)   (struct vnode *vp, void *buf, size_t len, int64_t *off);
    int (*vop_write)  (struct vnode *vp, const void *buf, size_t len,
                       int64_t *off);
    int (*vop_getattr)(struct vnode *vp, struct vattr *vap);
    int (*vop_setattr)(struct vnode *vp, struct vattr *vap);
    int (*vop_readdir)(struct vnode *vp, void *buf, size_t len, int64_t *off);
    int (*vop_reclaim)(struct vnode *vp);
    int (*vop_access) (struct vnode *vp, uint32_t mode, uint32_t uid);
};

/* The vnode */
struct vnode {
    vtype_t           v_type;
    uint32_t          v_flags;
    int               v_usecount;
    int               v_holdcnt;
    struct mount     *v_mount;
    struct vnode_ops *v_op;
    void             *v_data;      /* Filesystem-private (e.g. inode number) */
    spinlock_t        v_lock;
    struct list_head  v_list;      /* Global vnode list */
    struct list_head  v_mntlist;   /* Per-mount vnode list */
    uint64_t          v_hash_ino;  /* Inode for vfs_hash lookup */
    struct list_head  v_hashlist;  /* Entry in vfs_hash bucket */
};

/* VOP call macros */
#define VOP_LOOKUP(dvp, vpp, cnp) \
    ((dvp)->v_op->vop_lookup(dvp, vpp, cnp))
#define VOP_OPEN(vp, mode, uid) \
    ((vp)->v_op->vop_open(vp, mode, uid))
#define VOP_CLOSE(vp, mode, uid) \
    ((vp)->v_op->vop_close(vp, mode, uid))
#define VOP_GETATTR(vp, vap) \
    ((vp)->v_op->vop_getattr(vp, vap))
#define VOP_ACCESS(vp, mode, uid) \
    ((vp)->v_op->vop_access(vp, mode, uid))
#define VOP_READDIR(vp, buf, len, off) \
    ((vp)->v_op->vop_readdir(vp, buf, len, off))

static inline int VOP_READ(struct vnode *vp, void *buf, size_t len,
                            int64_t *off) {
    if (!vp || !vp->v_op->vop_read) return -ENOSYS;
    return vp->v_op->vop_read(vp, buf, len, off);
}

static inline int VOP_WRITE(struct vnode *vp, const void *buf, size_t len,
                             int64_t *off) {
    if (!vp || !vp->v_op->vop_write) return -ENOSYS;
    return vp->v_op->vop_write(vp, buf, len, off);
}

/* Vnode lifecycle */
int  getnewvnode(vtype_t type, struct mount *mp, struct vnode_ops *ops,
                 struct vnode **vpp);
void vref(struct vnode *vp);
void vrele(struct vnode *vp);
void vput(struct vnode *vp);
void vhold(struct vnode *vp);
void vdrop(struct vnode *vp);
void vgonel(struct vnode *vp);

/* Name cache */
void cache_init(void);
void cache_purge(struct vnode *vp);
int  cache_lookup(struct vnode *dvp, struct componentname *cnp,
                  struct vnode **vpp);
void cache_enter(struct vnode *dvp, struct vnode *vp,
                 struct componentname *cnp);

/* Vnode hash (inode deduplication) */
void vfs_hash_init(void);
int  vfs_hash_get(struct mount *mp, uint64_t ino, struct vnode **vpp);
void vfs_hash_insert(struct vnode *vp, uint64_t ino);
void vfs_hash_remove(struct vnode *vp);

/* VFS layer init */
void vfs_init(void);

#endif /* _LIBKERNEL_VFS_VNODE_H */
