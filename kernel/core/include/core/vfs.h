#ifndef _KERNEL_VFS_H
#define _KERNEL_VFS_H

#include <libkernel/types.h>

void vfs_resolve_path(const char *in, char *out, size_t size);

#endif
