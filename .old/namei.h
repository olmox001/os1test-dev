#ifndef _KERNEL_VFS_NAMEI_H
#define _KERNEL_VFS_NAMEI_H

#include <kernel/vfs/vnode.h>

/* componentname: encapsulates a single path component */
struct componentname {
    uint32_t    cn_flags;
    uid_t       cn_uid;
    const char  *cn_nameptr;    /* Pointer to start of component in path */
    size_t      cn_namelen;     /* Length of component */
};

/* nameidata: the state of a path lookup */
struct nameidata {
    const char          *ni_pathptr;    /* Remaining path to resolve */
    struct vnode        *ni_rootdir;    /* Root directory for lookup */
    struct vnode        *ni_startdir;   /* Starting directory (CWD or Root) */
    
    /* Result */
    struct vnode        *ni_vp;         /* Resulting vnode */
    struct vnode        *ni_dvp;        /* Parent of resulting vnode */
    
    struct componentname ni_cnd;
};

/* Namei Operations */
#define LOOKUP      0
#define CREATE      1
#define DELETE      2
#define RENAME      3

/* Namei Flags */
#define LOCKLEAF    0x0004
#define FOLLOW      0x0040
#define NOFOLLOW    0x0000

/* Path resolution */
int  namei(struct nameidata *ndp);
void NDINIT(struct nameidata *ndp, uint32_t op, uint32_t flags, const char *path);

/* Backward-compat: string-based lookup used by syscall_dispatch, process.c, main.c */
int  vfs_lookup(struct vnode *start_vp, const char *path, struct vnode **vpp, uid_t uid);

#endif /* _KERNEL_VFS_NAMEI_H */
