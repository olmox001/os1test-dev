/*
 * kernel/libkernel/src/vfs/vfs_lookup.c
 * Path resolution: namei(), NDINIT(), vfs_lookup(), vfs_resolve_path().
 */
#include <libkernel/vfs/vnode.h>
#include <libkernel/vfs/mount.h>
#include <libkernel/vfs/namei.h>
#include <core/sched.h>
#include <core/string.h>

extern int cache_lookup(struct vnode *dvp, struct componentname *cnp,
                        struct vnode **vpp);
extern void cache_enter(struct vnode *dvp, struct vnode *vp,
                        struct componentname *cnp);

int namei(struct nameidata *ndp) {
    const char   *p   = ndp->ni_pathptr;
    struct vnode *cvp = (*p == '/') ? ndp->ni_rootdir : ndp->ni_startdir;

    if (!cvp) return -ENOENT;
    vref(cvp);

    while (*p == '/') p++;

    while (*p) {
        char component[256];
        int  i = 0;
        while (*p && *p != '/') {
            if (i < 255) component[i++] = *p;
            p++;
        }
        component[i] = '\0';
        while (*p == '/') p++;
        if (i == 0) break;

        struct componentname *cnp = &ndp->ni_cnd;
        cnp->cn_nameptr = component;
        cnp->cn_namelen = (size_t)i;

        struct vnode *nvp = NULL;
        if (cache_lookup(cvp, cnp, &nvp)) {
            vrele(cvp);
            cvp = nvp;
            continue;
        }

        if (!cvp->v_op || !cvp->v_op->vop_lookup) {
            vrele(cvp);
            return -ENOTDIR;
        }

        int err = VOP_LOOKUP(cvp, &nvp, cnp);
        if (err) { vrele(cvp); return err; }

        cache_enter(cvp, nvp, cnp);
        vrele(cvp);
        cvp = nvp;
    }

    ndp->ni_vp = cvp;
    return 0;
}

void NDINIT(struct nameidata *ndp, uint32_t op __unused, uint32_t flags,
            const char *path) {
    struct vnode *root = vfs_get_root();
    ndp->ni_pathptr      = path;
    ndp->ni_rootdir      = root;
    ndp->ni_startdir     = root;
    ndp->ni_cnd.cn_flags = flags;
    ndp->ni_cnd.cn_uid   = 0;
    ndp->ni_vp           = NULL;
    ndp->ni_dvp          = NULL;
}

int vfs_lookup(struct vnode *start_vp, const char *path, struct vnode **vpp,
               uint32_t uid __unused) {
    if (!path || !*path) return -EINVAL;

    struct nameidata nd;
    NDINIT(&nd, LOOKUP, NOFOLLOW, path);

    if (start_vp && *path != '/')
        nd.ni_startdir = start_vp;

    int err = namei(&nd);
    if (!err) *vpp = nd.ni_vp;
    return err;
}

void vfs_resolve_path(const char *in, char *out, size_t size) {
    if (!in || !out || size == 0) return;

    char tmp[256];
    size_t pos = 0;

    if (in[0] != '/') {
        const char *cwd = (current_process && current_process->cwd[0])
                          ? current_process->cwd : "/";
        size_t clen = strlen(cwd);
        if (clen < sizeof(tmp) - 2) {
            memcpy(tmp, cwd, clen);
            pos = clen;
            if (tmp[pos - 1] != '/') tmp[pos++] = '/';
        }
    }

    size_t ilen = strlen(in);
    if (pos + ilen + 1 < sizeof(tmp)) {
        memcpy(tmp + pos, in, ilen);
        pos += ilen;
    }
    tmp[pos] = '\0';

    out[0] = '/';
    size_t olen = 1;
    const char *p = tmp;
    while (*p) {
        while (*p == '/') p++;
        if (!*p) break;

        const char *comp = p;
        while (*p && *p != '/') p++;
        size_t clen = (size_t)(p - comp);

        if (clen == 1 && comp[0] == '.') {
            continue;
        } else if (clen == 2 && comp[0] == '.' && comp[1] == '.') {
            if (olen > 1) {
                olen--;
                while (olen > 1 && out[olen - 1] != '/') olen--;
            }
        } else {
            if (olen + clen + 2 >= size) break;
            if (olen > 1) out[olen++] = '/';
            memcpy(out + olen, comp, clen);
            olen += clen;
        }
    }
    out[olen] = '\0';
}
