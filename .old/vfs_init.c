/*
 * kernel/fs/vfs/vfs_init.c
 * VFS layer initialization entry point
 */
#include <kernel/vfs/vnode.h>

extern void ext4_vfs_init(void);

void vfs_init(void) {
    cache_init();
    vfs_hash_init();
    ext4_vfs_init();
}
