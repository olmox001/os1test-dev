#include <kernel/vfs/vnode.h>
#include <kernel/vfs/namei.h>
#include <kernel/kmalloc.h>
#include <kernel/string.h>

#define CACHE_SIZE 1024
#define CACHE_HASH(dvp, name, len) (((uintptr_t)(dvp) ^ (uintptr_t)(name)[0]) % CACHE_SIZE)

struct cache_entry {
    struct vnode        *dvp;
    char                name[64];
    size_t              len;
    struct vnode        *vp;
    struct list_head    list;
};

static struct list_head cache_hash[CACHE_SIZE];
static DEFINE_SPINLOCK(cache_lock);

void cache_init(void) {
    for (int i = 0; i < CACHE_SIZE; i++) {
        INIT_LIST_HEAD(&cache_hash[i]);
    }
}

int cache_lookup(struct vnode *dvp, struct componentname *cnp, struct vnode **vpp) {
    uint32_t hash = CACHE_HASH(dvp, cnp->cn_nameptr, cnp->cn_namelen);
    struct cache_entry *ce;
    
    spin_lock(&cache_lock);
    list_for_each_entry(ce, &cache_hash[hash], list) {
        if (ce->dvp == dvp && ce->len == cnp->cn_namelen && 
            memcmp(ce->name, cnp->cn_nameptr, ce->len) == 0) {
            *vpp = ce->vp;
            vref(ce->vp);
            spin_unlock(&cache_lock);
            return 1; /* Found */
        }
    }
    spin_unlock(&cache_lock);
    return 0; /* Not found */
}

void cache_enter(struct vnode *dvp, struct vnode *vp, struct componentname *cnp) {
    if (cnp->cn_namelen >= 64) return;
    
    uint32_t hash = CACHE_HASH(dvp, cnp->cn_nameptr, cnp->cn_namelen);
    struct cache_entry *ce = kmalloc(sizeof(struct cache_entry));
    if (!ce) return;
    
    ce->dvp = dvp;
    ce->vp = vp;
    ce->len = cnp->cn_namelen;
    memcpy(ce->name, cnp->cn_nameptr, ce->len);
    ce->name[ce->len] = '\0';
    
    spin_lock(&cache_lock);
    list_add(&ce->list, &cache_hash[hash]);
    spin_unlock(&cache_lock);
}

void cache_purge(struct vnode *vp __unused) {
    spin_lock(&cache_lock);
    /* In a real implementation we'd scan and remove all entries pointing to or from vp */
    spin_unlock(&cache_lock);
}
