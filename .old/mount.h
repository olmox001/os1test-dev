#ifndef _KERNEL_VFS_MOUNT_H
#define _KERNEL_VFS_MOUNT_H

#include <kernel/types.h>
#include <kernel/list.h>
#include <kernel/spinlock.h>

struct vnode;

struct mount {
    struct list_head    mnt_list;       /* Global list of mount points */
    spinlock_t          mnt_lock;
    struct vfsops       *mnt_op;        /* Filesystem operations */
    struct vnode        *mnt_vnodecovered; /* Vnode we are mounted on */
    struct vnode        *mnt_rootvnode;    /* Root vnode of this mount */
    void                *mnt_data;         /* Filesystem private data */
    uint32_t            mnt_flag;
    char                mnt_stat[64];      /* Filesystem type name */
};

/* Mount Flags */
#define MNT_RDONLY      0x00000001
#define MNT_NOSUID      0x00000008
#define MNT_ROOTFS      0x00004000

struct vfsops {
    int (*vfs_mount)(struct mount *mp);
    int (*vfs_unmount)(struct mount *mp, int flags);
    int (*vfs_root)(struct mount *mp, struct vnode **vpp);
    int (*vfs_statfs)(struct mount *mp, void *sbp);
};

/* Mount management */
int          vfs_mount(const char *type, struct vnode *covered, uint32_t flags, void *data, struct vfsops *ops);
void         vfs_set_root(struct vnode *vp);
struct vnode *vfs_get_root(void);

#endif /* _KERNEL_VFS_MOUNT_H */
