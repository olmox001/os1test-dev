#include <kernel/vfs/vnode.h>
#include <kernel/vfs/namei.h>
#include <kernel/vfs/mount.h>
#include <kernel/sched.h>
#include <kernel/cpu.h>
#include <kernel/string.h>

extern int cache_lookup(struct vnode *dvp, struct componentname *cnp, struct vnode **vpp);
extern void cache_enter(struct vnode *dvp, struct vnode *vp, struct componentname *cnp);

/*
 * namei: the main entry point for path resolution.
 */
int namei(struct nameidata *ndp) {
    const char *p = ndp->ni_pathptr;
    struct vnode *cvp;
    
    /* Determine start directory */
    if (*p == '/') {
        cvp = ndp->ni_rootdir;
        while (*p == '/') p++;
    } else {
        cvp = ndp->ni_startdir;
    }
    
    if (!cvp) return -ENOENT;
    vref(cvp);
    
    while (*p) {
        char component[256];
        int i = 0;
        while (*p && *p != '/') {
            if (i < 255) component[i++] = *p;
            p++;
        }
        component[i] = '\0';
        while (*p == '/') p++;
        
        if (i == 0) break; /* Trailing slash */
        
        struct componentname *cnp = &ndp->ni_cnd;
        cnp->cn_nameptr = component;
        cnp->cn_namelen = i;
        
        /* Check Namecache */
        struct vnode *nvp = NULL;
        if (cache_lookup(cvp, cnp, &nvp)) {
            vrele(cvp);
            cvp = nvp;
            continue;
        }
        
        /* Call Filesystem Lookup */
        int error = VOP_LOOKUP(cvp, &nvp, cnp);
        if (error) {
            vrele(cvp);
            return error;
        }
        
        /* Enter into cache */
        cache_enter(cvp, nvp, cnp);
        
        vrele(cvp);
        cvp = nvp;
        
        /* Handle Mount Points */
        if (cvp->v_type == VDIR && cvp->v_mntlist.next != &cvp->v_mntlist) {
            /* If this is a mount point, traverse to the mounted root */
            /* (Simplified for now: assumes no nested mounts) */
        }
    }
    
    ndp->ni_vp = cvp;
    return 0;
}

/*
 * NDINIT: Initialize nameidata for a lookup.
 */
void NDINIT(struct nameidata *ndp, uint32_t op __unused, uint32_t flags, const char *path) {
    ndp->ni_pathptr = path;
    ndp->ni_cnd.cn_flags = flags;
    
    struct cpu_info *cpu = get_cpu_info();
    if (cpu && cpu->current_task) {
        ndp->ni_rootdir = cpu->current_task->root_vn;
        ndp->ni_startdir = cpu->current_task->cwd_vn;
    } else {
        extern struct vnode *vfs_get_root(void);
        ndp->ni_rootdir = vfs_get_root();
        ndp->ni_startdir = vfs_get_root();
    }
    
    ndp->ni_cnd.cn_uid = (cpu && cpu->current_task) ? cpu->current_task->uid : 0;
}
