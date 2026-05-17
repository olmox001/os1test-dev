/*
 * kernel/core/src/fs/vfs_ext4.c
 * VFS adaptor for the boot Ext4 filesystem (boot_fs.c).
 *
 * Phase 3a strategy: path resolution uses boot_fs_find_inode() directly
 * rather than a component-by-component VOP_LOOKUP walk, because boot_fs.c
 * exposes a full-path API rather than per-directory lookup.  The full vnode
 * hierarchy (VOP_LOOKUP in every directory) is a Phase 3b extension.
 *
 * Vnode private data: uint32_t inode number stored as (void *)(uintptr_t)ino.
 */
#include <core/vfs.h>
#include <core/boot_fs.h>
#include <core/kmalloc.h>
#include <core/printk.h>
#include <core/string.h>

/* ----- vnode ops --------------------------------------------------------- */

static int ext4_vop_open(struct vnode *vp __unused, int mode __unused,
                         uint32_t uid __unused) {
    return 0;
}

static int ext4_vop_close(struct vnode *vp __unused, int mode __unused,
                          uint32_t uid __unused) {
    return 0;
}

static int ext4_vop_read(struct vnode *vp, void *buf, size_t len,
                         int64_t *off) {
    uint32_t ino = (uint32_t)(uintptr_t)vp->v_data;
    int bytes = boot_fs_read_inode(ino, (uint64_t)*off, (uint8_t *)buf,
                                   (uint32_t)len);
    if (bytes > 0)
        *off += bytes;
    return bytes;
}

static int ext4_vop_readdir(struct vnode *vp __unused, void *buf __unused,
                             size_t len __unused, int64_t *off __unused) {
    return -ENOSYS; /* Phase 3b */
}

static int ext4_vop_reclaim(struct vnode *vp) {
    vp->v_data = NULL;
    return 0;
}

static struct vnode_ops ext4_vops = {
    .vop_lookup  = NULL,
    .vop_create  = NULL,
    .vop_open    = ext4_vop_open,
    .vop_close   = ext4_vop_close,
    .vop_read    = ext4_vop_read,
    .vop_write   = NULL,
    .vop_getattr = NULL,
    .vop_setattr = NULL,
    .vop_readdir = ext4_vop_readdir,
    .vop_reclaim = ext4_vop_reclaim,
    .vop_access  = NULL,
};

/* ----- vfsops ------------------------------------------------------------ */

static struct mount *ext4_mount_ptr = NULL;

static int ext4_vfs_mount_op(struct mount *mp) {
    ext4_mount_ptr = mp;
    return 0;
}

static int ext4_vfs_unmount(struct mount *mp __unused, int flags __unused) {
    return 0;
}

static int ext4_vfs_root(struct mount *mp, struct vnode **vpp) {
    /* Root inode of Ext4 is always inode 2. */
    struct vnode *vp = NULL;

    if (vfs_hash_get(mp, 2, &vp) == 0) {
        *vpp = vp;
        return 0;
    }

    int err = getnewvnode(VDIR, mp, &ext4_vops, &vp);
    if (err) return err;

    vp->v_data = (void *)(uintptr_t)2;
    vfs_hash_insert(vp, 2);
    *vpp = vp;
    return 0;
}

static struct vfsops ext4_vfsops = {
    .vfs_mount   = ext4_vfs_mount_op,
    .vfs_unmount = ext4_vfs_unmount,
    .vfs_root    = ext4_vfs_root,
    .vfs_statfs  = NULL,
};

/* ----- open helper used by sys_open ------------------------------------- */

/*
 * ext4_open_path: resolve a full path and return a vnode.
 * On success *vpp holds a reference that the caller must vrele().
 */
int ext4_open_path(const char *path, struct vnode **vpp) {
    if (!ext4_mount_ptr) return -ENXIO;

    uint32_t ino = boot_fs_find_inode(path);
    if (!ino) return -ENOENT;

    struct vnode *vp = NULL;
    if (vfs_hash_get(ext4_mount_ptr, (uint64_t)ino, &vp) == 0) {
        *vpp = vp;
        return 0;
    }

    int err = getnewvnode(VREG, ext4_mount_ptr, &ext4_vops, &vp);
    if (err) return err;

    vp->v_data = (void *)(uintptr_t)ino;
    vfs_hash_insert(vp, (uint64_t)ino);
    *vpp = vp;
    return 0;
}

/* ----- init -------------------------------------------------------------- */

void ext4_vfs_init(void) {
    vfs_mount("ext4", NULL, MNT_RDONLY | MNT_ROOTFS, NULL, &ext4_vfsops);
    pr_info("VFS: ext4 root mounted (read-only)\n");
}
