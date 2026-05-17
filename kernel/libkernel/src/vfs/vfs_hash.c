/*
 * kernel/libkernel/src/vfs/vfs_hash.c
 * Vnode hash table: (mount, inode) → vnode deduplication.
 */
#include <libkernel/vfs/vnode.h>
#include <libkernel/vfs/mount.h>
#include <core/spinlock.h>
#include <libkernel/list.h>

#define VFS_HASH_SIZE 512

static struct list_head vfs_hash_table[VFS_HASH_SIZE];
static DEFINE_SPINLOCK(vfs_hash_lock);

static uint32_t vhash(struct mount *mp, uint64_t ino) {
    return (uint32_t)(((uintptr_t)mp >> 4) ^ ino) % VFS_HASH_SIZE;
}

void vfs_hash_init(void) {
    for (int i = 0; i < VFS_HASH_SIZE; i++)
        INIT_LIST_HEAD(&vfs_hash_table[i]);
}

int vfs_hash_get(struct mount *mp, uint64_t ino, struct vnode **vpp) {
    uint32_t idx = vhash(mp, ino);
    struct vnode *vp;

    spin_lock(&vfs_hash_lock);
    list_for_each_entry(vp, &vfs_hash_table[idx], v_hashlist) {
        if (vp->v_mount == mp && vp->v_hash_ino == ino &&
            !(vp->v_flags & VDOOMED)) {
            vp->v_usecount++;
            *vpp = vp;
            spin_unlock(&vfs_hash_lock);
            return 0;
        }
    }
    spin_unlock(&vfs_hash_lock);
    return -ENOENT;
}

void vfs_hash_insert(struct vnode *vp, uint64_t ino) {
    vp->v_hash_ino = ino;
    INIT_LIST_HEAD(&vp->v_hashlist);
    uint32_t idx = vhash(vp->v_mount, ino);

    spin_lock(&vfs_hash_lock);
    list_add(&vp->v_hashlist, &vfs_hash_table[idx]);
    spin_unlock(&vfs_hash_lock);
}

void vfs_hash_remove(struct vnode *vp) {
    spin_lock(&vfs_hash_lock);
    if (!list_empty(&vp->v_hashlist))
        list_del_init(&vp->v_hashlist);
    spin_unlock(&vfs_hash_lock);
}
