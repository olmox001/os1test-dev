#ifndef _KERNEL_VFS_VNODE_H
#define _KERNEL_VFS_VNODE_H

#include <kernel/types.h>
#include <kernel/list.h>
#include <kernel/spinlock.h>

/* Vnode Types */
typedef enum vtype {
    VNON, VREG, VDIR, VBLK, VCHR, VLNK, VSOCK, VFIFO, VBAD, VMARKER
} vtype_t;

/* Vnode Flags */
#define VROOT       0x0001  /* Root of its filesystem */
#define VTEXT       0x0002  /* Vnode is a pure text prototype */
#define VSYSTEM     0x0004  /* Vnode being used by kernel */
#define VDOOMED     0x0008  /* Vnode is being recycled */

/* Vnode Attribute Structure (vattr) */
struct vattr {
    vtype_t     va_type;
    mode_t      va_mode;
    nlink_t     va_nlink;
    uid_t       va_uid;
    gid_t       va_gid;
    uint64_t    va_fsid;
    uint64_t    va_fileid;
    uint64_t    va_size;
    time_t      va_atime;
    time_t      va_mtime;
    time_t      va_ctime;
    uint32_t    va_blocksize;
    uint64_t    va_bytes;
};

struct vnode;
struct componentname;
struct vattr;
struct mount;

/* Vnode Operations (VOPs) */
struct vnode_ops {
    int (*vop_lookup)(struct vnode *dvp, struct vnode **vpp, struct componentname *cnp);
    int (*vop_create)(struct vnode *dvp, struct vnode **vpp, struct componentname *cnp, struct vattr *vap);
    int (*vop_open)(struct vnode *vp, int mode, uid_t uid);
    int (*vop_close)(struct vnode *vp, int mode, uid_t uid);
    int (*vop_read)(struct vnode *vp, void *buf, size_t len, off_t *off);
    int (*vop_write)(struct vnode *vp, const void *buf, size_t len, off_t *off);
    int (*vop_getattr)(struct vnode *vp, struct vattr *vap);
    int (*vop_setattr)(struct vnode *vp, struct vattr *vap);
    int (*vop_readdir)(struct vnode *vp, void *buf, size_t len, off_t *off);
    int (*vop_reclaim)(struct vnode *vp);
    int (*vop_access)(struct vnode *vp, mode_t mode, uid_t uid);
};

/* The Vnode Structure */
struct vnode {
    vtype_t             v_type;
    uint32_t            v_flags;
    int                 v_usecount;
    int                 v_holdcnt;
    struct mount        *v_mount;
    struct vnode_ops    *v_op;
    void                *v_data;        /* Filesystem specific data (e.g., ext4_inode) */
    spinlock_t          v_lock;
    struct list_head    v_list;         /* List of all vnodes in system */
    struct list_head    v_mntlist;      /* List of vnodes in a mount point */
    uint64_t            v_hash_ino;     /* Inode number for vfs_hash lookup */
    struct list_head    v_hashlist;     /* Entry in vfs_hash bucket */
};

/* VOP Macros */
#define VOP_LOOKUP(dvp, vpp, cnp)   ((dvp)->v_op->vop_lookup(dvp, vpp, cnp))
#define VOP_OPEN(vp, mode, uid)     ((vp)->v_op->vop_open(vp, mode, uid))
#define VOP_CLOSE(vp, mode, uid)    ((vp)->v_op->vop_close(vp, mode, uid))
#define VOP_GETATTR(vp, vap)        ((vp)->v_op->vop_getattr(vp, vap))
#define VOP_ACCESS(vp, mode, uid)   ((vp)->v_op->vop_access(vp, mode, uid))

static inline int VOP_READ(struct vnode *vp, void *buf, size_t len, off_t *off) {
    if (!vp->v_op->vop_read) return -ENOSYS;
    return vp->v_op->vop_read(vp, buf, len, off);
}

static inline int VOP_WRITE(struct vnode *vp, const void *buf, size_t len, off_t *off) {
    if (!vp->v_op->vop_write) return -ENOSYS;
    return vp->v_op->vop_write(vp, buf, len, off);
}

#define VOP_READDIR(vp, buf, len, off) ((vp)->v_op->vop_readdir(vp, buf, len, off))

/* Vnode lifecycle */
int  getnewvnode(vtype_t type, struct mount *mp, struct vnode_ops *ops, struct vnode **vpp);
void vref(struct vnode *vp);
void vrele(struct vnode *vp);
void vput(struct vnode *vp);
void vhold(struct vnode *vp);
void vdrop(struct vnode *vp);
void vgonel(struct vnode *vp);

/* Namecache */
void cache_init(void);
void cache_purge(struct vnode *vp);
int  cache_lookup(struct vnode *dvp, struct componentname *cnp, struct vnode **vpp);
void cache_enter(struct vnode *dvp, struct vnode *vp, struct componentname *cnp);

/* Vnode hash (vfs_hash.c) */
void vfs_hash_init(void);
int  vfs_hash_get(struct mount *mp, uint64_t ino, struct vnode **vpp);
void vfs_hash_insert(struct vnode *vp, uint64_t ino);
void vfs_hash_remove(struct vnode *vp);

/* VFS layer init */
void vfs_init(void);

#endif /* _KERNEL_VFS_VNODE_H */
