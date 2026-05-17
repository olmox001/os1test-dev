/*
 * kernel/libkernel/include/libkernel/vfs/namei.h
 * Path resolution structures and prototypes (libkernel).
 */
#ifndef _LIBKERNEL_VFS_NAMEI_H
#define _LIBKERNEL_VFS_NAMEI_H

#include <libkernel/vfs/vnode.h>

/* Single path component */
struct componentname {
    uint32_t    cn_flags;
    uint32_t    cn_uid;
    const char *cn_nameptr;
    size_t      cn_namelen;
};

/* State for a full path resolution */
struct nameidata {
    const char         *ni_pathptr;
    struct vnode       *ni_rootdir;
    struct vnode       *ni_startdir;
    struct vnode       *ni_vp;
    struct vnode       *ni_dvp;
    struct componentname ni_cnd;
};

/* Operations */
#define LOOKUP  0
#define CREATE  1
#define DELETE  2
#define RENAME  3

/* Flags */
#define LOCKLEAF  0x0004
#define FOLLOW    0x0040
#define NOFOLLOW  0x0000

void NDINIT(struct nameidata *ndp, uint32_t op, uint32_t flags,
            const char *path);
int  namei(struct nameidata *ndp);
int  vfs_lookup(struct vnode *start_vp, const char *path, struct vnode **vpp,
                uint32_t uid);
void vfs_resolve_path(const char *in, char *out, size_t size);

#endif /* _LIBKERNEL_VFS_NAMEI_H */
