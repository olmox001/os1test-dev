#include <kernel/vfs/vnode.h>
#include <kernel/vfs/mount.h>
#include <kernel/kmalloc.h>
#include <kernel/string.h>
#include <kernel/printk.h>

static LIST_HEAD(vnode_list_head);
static DEFINE_SPINLOCK(vnode_list_lock);

/*
 * Allocate a new vnode for a filesystem.
 */
int getnewvnode(vtype_t type, struct mount *mp, struct vnode_ops *ops, struct vnode **vpp) {
    struct vnode *vp = kmalloc(sizeof(struct vnode));
    if (!vp) return -ENOMEM;
    
    memset(vp, 0, sizeof(struct vnode));
    vp->v_type = type;
    vp->v_mount = mp;
    vp->v_op = ops;
    vp->v_usecount = 1;
    vp->v_holdcnt = 1;
    spin_lock_init(&vp->v_lock);
    INIT_LIST_HEAD(&vp->v_list);
    INIT_LIST_HEAD(&vp->v_mntlist);
    
    uint64_t flags;
    spin_lock_irqsave(&vnode_list_lock, &flags);
    list_add_tail(&vp->v_list, &vnode_list_head);
    spin_unlock_irqrestore(&vnode_list_lock, flags);
    
    if (mp) {
        /* Add to mount's vnode list if needed */
    }
    
    *vpp = vp;
    return 0;
}

/*
 * vput: unlock and release a vnode.
 */
void vput(struct vnode *vp) {
    if (!vp) return;
    /* In OS1 we don't have complex vnode locking yet, so just release */
    vrele(vp);
}

/*
 * vrele: release a vnode reference.
 */
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

/*
 * vhold: add a hold reference (prevents recycling but not reclamation).
 */
void vhold(struct vnode *vp) {
    spin_lock(&vp->v_lock);
    vp->v_holdcnt++;
    spin_unlock(&vp->v_lock);
}

/*
 * vdrop: release a hold reference.
 */
void vdrop(struct vnode *vp) {
    spin_lock(&vp->v_lock);
    vp->v_holdcnt--;
    if (vp->v_holdcnt <= 0 && vp->v_usecount <= 0) {
        spin_unlock(&vp->v_lock);
        /* Actually free the memory */
        uint64_t flags;
        spin_lock_irqsave(&vnode_list_lock, &flags);
        list_del(&vp->v_list);
        spin_unlock_irqrestore(&vnode_list_lock, flags);
        kfree(vp);
        return;
    }
    spin_unlock(&vp->v_lock);
}

/*
 * vgonel: prepare vnode for destruction.
 * Remove from vfs_hash first so no new lookup can find it,
 * then call the filesystem's reclaim (which frees v_data),
 * then release the hold reference.
 */
void vgonel(struct vnode *vp) {
    vfs_hash_remove(vp);
    vp->v_flags |= VDOOMED;
    if (vp->v_op && vp->v_op->vop_reclaim) {
        vp->v_op->vop_reclaim(vp);
    }
    vdrop(vp);
}

/*
 * vref: add a reference.
 */
void vref(struct vnode *vp) {
    spin_lock(&vp->v_lock);
    vp->v_usecount++;
    spin_unlock(&vp->v_lock);
}
