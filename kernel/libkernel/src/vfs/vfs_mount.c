/*
 * kernel/libkernel/src/vfs/vfs_mount.c
 * VFS mount table management (libkernel).
 */
#include <libkernel/vfs/vnode.h>
#include <libkernel/vfs/mount.h>
#include <core/kmalloc.h>
#include <core/string.h>

static LIST_HEAD(mount_list);
static DEFINE_SPINLOCK(mount_list_lock);
static struct vnode *root_vnode     = NULL;
static struct mount *root_mount_ptr = NULL;

int vfs_mount(const char *type, struct vnode *covered, uint32_t flags,
              void *data, struct vfsops *ops) {
    struct mount *mp = kmalloc(sizeof(struct mount));
    if (!mp) return -ENOMEM;

    memset(mp, 0, sizeof(struct mount));
    strncpy(mp->mnt_stat, type, sizeof(mp->mnt_stat) - 1);
    mp->mnt_flag         = flags;
    mp->mnt_data         = data;
    mp->mnt_op           = ops;
    mp->mnt_vnodecovered = covered;
    spin_lock_init(&mp->mnt_lock);
    INIT_LIST_HEAD(&mp->mnt_list);

    int err = ops->vfs_mount(mp);
    if (err) { kfree(mp); return err; }

    spin_lock(&mount_list_lock);
    list_add_tail(&mp->mnt_list, &mount_list);
    spin_unlock(&mount_list_lock);

    if (flags & MNT_ROOTFS) {
        ops->vfs_root(mp, &root_vnode);
        root_mount_ptr = mp;
        if (root_vnode) root_vnode->v_flags |= VROOT;
    }

    return 0;
}

void vfs_set_root(struct vnode *vp)   { root_vnode     = vp; }
struct vnode *vfs_get_root(void)      { return root_vnode; }
struct mount *vfs_get_root_mount(void){ return root_mount_ptr; }
