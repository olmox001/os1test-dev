/*
 * kernel/libkernel/src/vfs/vfs_subr.c
 * Vnode lifecycle: allocation, reference counting, reclaim.
 */
#include <libkernel/vfs/vnode.h>
#include <libkernel/vfs/mount.h>
#include <core/kmalloc.h>
#include <core/string.h>

static LIST_HEAD(vnode_list_head);
static DEFINE_SPINLOCK(vnode_list_lock);

int getnewvnode(vtype_t type, struct mount *mp, struct vnode_ops *ops,
                struct vnode **vpp) {
    struct vnode *vp = kmalloc(sizeof(struct vnode));
    if (!vp) return -ENOMEM;

    memset(vp, 0, sizeof(struct vnode));
    vp->v_type     = type;
    vp->v_mount    = mp;
    vp->v_op       = ops;
    vp->v_usecount = 1;
    vp->v_holdcnt  = 1;
    spin_lock_init(&vp->v_lock);
    INIT_LIST_HEAD(&vp->v_list);
    INIT_LIST_HEAD(&vp->v_mntlist);
    INIT_LIST_HEAD(&vp->v_hashlist);

    uint64_t flags;
    spin_lock_irqsave(&vnode_list_lock, &flags);
    list_add_tail(&vp->v_list, &vnode_list_head);
    spin_unlock_irqrestore(&vnode_list_lock, flags);

    *vpp = vp;
    return 0;
}

void vref(struct vnode *vp) {
    if (!vp) return;
    spin_lock(&vp->v_lock);
    vp->v_usecount++;
    spin_unlock(&vp->v_lock);
}

void vrele(struct vnode *vp) {
    if (!vp) return;
    spin_lock(&vp->v_lock);
    vp->v_usecount--;
    if (vp->v_usecount <= 0) {
        spin_unlock(&vp->v_lock);
        vgonel(vp);
        return;
    }
    spin_unlock(&vp->v_lock);
}

void vput(struct vnode *vp) {
    if (!vp) return;
    vrele(vp);
}

void vhold(struct vnode *vp) {
    if (!vp) return;
    spin_lock(&vp->v_lock);
    vp->v_holdcnt++;
    spin_unlock(&vp->v_lock);
}

void vdrop(struct vnode *vp) {
    if (!vp) return;
    spin_lock(&vp->v_lock);
    vp->v_holdcnt--;
    if (vp->v_holdcnt <= 0 && vp->v_usecount <= 0) {
        spin_unlock(&vp->v_lock);
        uint64_t flags;
        spin_lock_irqsave(&vnode_list_lock, &flags);
        list_del(&vp->v_list);
        spin_unlock_irqrestore(&vnode_list_lock, flags);
        kfree(vp);
        return;
    }
    spin_unlock(&vp->v_lock);
}

void vgonel(struct vnode *vp) {
    vfs_hash_remove(vp);
    vp->v_flags |= VDOOMED;
    if (vp->v_op && vp->v_op->vop_reclaim)
        vp->v_op->vop_reclaim(vp);
    vdrop(vp);
}
