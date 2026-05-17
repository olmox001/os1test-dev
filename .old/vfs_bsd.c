/*
 * kernel/fs/vfs_bsd.c
 * Backward-compat string-based path lookup (vfs_lookup).
 * Full vnode lifecycle and mount management live in kernel/fs/vfs/.
 */
#include <kernel/vfs/vnode.h>
#include <kernel/vfs/mount.h>
#include <kernel/vfs/namei.h>
#include <kernel/string.h>
#include <kernel/sched.h>
#include <kernel/cpu.h>

/*
 * vfs_lookup: walk a path component-by-component starting from start_vp.
 * Absolute paths (starting with '/') are anchored at the current process
 * root_vn, falling back to the global VFS root.
 */
int vfs_lookup(struct vnode *start_vp, const char *path, struct vnode **vpp, uid_t uid) {
    if (!path || !*path) return -EINVAL;

    struct vnode *current_vp = start_vp;
    char name[256];
    const char *p = path;

    if (*p == '/') {
        struct vnode *root = vfs_get_root();

        struct cpu_info *cpu = get_cpu_info();
        if (cpu && cpu->current_task && cpu->current_task->root_vn)
            root = cpu->current_task->root_vn;

        current_vp = root;
        while (*p == '/') p++;
    }

    if (!current_vp) return -ENOENT;

    while (*p) {
        int i = 0;
        while (*p && *p != '/') {
            if (i < 255) name[i++] = *p;
            p++;
        }
        name[i] = '\0';
        while (*p == '/') p++;

        if (i == 0) break;

        struct componentname cn;
        cn.cn_nameptr = name;
        cn.cn_namelen = (size_t)i;
        cn.cn_uid     = uid;
        cn.cn_flags   = 0;

        if (!current_vp->v_op->vop_lookup) return -ENOTDIR;

        if (current_vp->v_op->vop_access) {
            int acc_err = current_vp->v_op->vop_access(current_vp, 01, uid);
            if (acc_err != 0) return acc_err;
        }

        struct vnode *next_vp = NULL;
        int err = current_vp->v_op->vop_lookup(current_vp, &next_vp, &cn);
        if (err != 0) return err;

        current_vp = next_vp;
    }

    *vpp = current_vp;
    return 0;
}
