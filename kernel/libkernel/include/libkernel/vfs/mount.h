/*
 * kernel/libkernel/include/libkernel/vfs/mount.h
 * VFS mount point and filesystem operations (libkernel).
 */
#ifndef _LIBKERNEL_VFS_MOUNT_H
#define _LIBKERNEL_VFS_MOUNT_H

#include <libkernel/types.h>
#include <libkernel/list.h>
#include <core/spinlock.h>

struct vnode;

struct vfsops {
    int (*vfs_mount)  (struct mount *mp);
    int (*vfs_unmount)(struct mount *mp, int flags);
    int (*vfs_root)   (struct mount *mp, struct vnode **vpp);
    int (*vfs_statfs) (struct mount *mp, void *sbp);
};

struct mount {
    struct list_head  mnt_list;
    spinlock_t        mnt_lock;
    struct vfsops    *mnt_op;
    struct vnode     *mnt_vnodecovered;
    struct vnode     *mnt_rootvnode;
    void             *mnt_data;
    uint32_t          mnt_flag;
    char              mnt_stat[64];
};

#define MNT_RDONLY  0x00000001
#define MNT_NOSUID  0x00000008
#define MNT_ROOTFS  0x00004000

int           vfs_mount(const char *type, struct vnode *covered,
                        uint32_t flags, void *data, struct vfsops *ops);
void          vfs_set_root(struct vnode *vp);
struct vnode *vfs_get_root(void);
struct mount *vfs_get_root_mount(void);

#endif /* _LIBKERNEL_VFS_MOUNT_H */
