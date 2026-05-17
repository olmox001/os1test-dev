/*
 * kernel/libkernel/src/vfs/vfs_init.c
 * VFS layer initialization entry point (libkernel).
 */
#include <libkernel/vfs/vnode.h>
#include <core/printk.h>

extern void ext4_vfs_init(void);

void vfs_init(void) {
    cache_init();
    vfs_hash_init();
    ext4_vfs_init();
    pr_info("VFS: layer initialized\n");
}
