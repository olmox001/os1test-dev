#ifndef _CORE_BOOT_FS_H
#define _CORE_BOOT_FS_H

#include <libkernel/types.h>

int      boot_fs_init(void);
uint32_t ext4_find_inode(const char *path);
int      ext4_read_inode(uint32_t ino, uint64_t offset, uint8_t *buf, uint32_t size);
int      ext4_list_dir(const char *path, char *buf, size_t size);

#endif /* _CORE_BOOT_FS_H */
