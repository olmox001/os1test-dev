/*
 * kernel/libkernel/src/registry.c
 * Hierarchical Key-Value Registry with Per-Key IPC Queues
 * Plan 9 + seL4 style: everything is a registry node.
 */

#include <core/printk.h>
#include <core/registry.h>
#include <core/kmalloc.h>
#include <core/spinlock.h>
#include <core/sched.h>
#include <core/vmm.h>
#include <libkernel/string.h>
#include <stdbool.h>

/* ================================================================
   Static Node Pool — no heap fragmentation for kernel data
   ================================================================ */

static struct reg_node  node_pool[REG_POOL_SIZE];
static uint8_t          pool_used[REG_POOL_SIZE];
static DEFINE_SPINLOCK(pool_lock);
static DEFINE_SPINLOCK(tree_lock);

static struct reg_node *reg_root;

static struct reg_node *alloc_node(void) {
    uint64_t flags;
    spin_lock_irqsave(&pool_lock, &flags);
    for (int i = 0; i < REG_POOL_SIZE; i++) {
        if (!pool_used[i]) {
            pool_used[i] = 1;
            memset(&node_pool[i], 0, sizeof(struct reg_node));
            spin_unlock_irqrestore(&pool_lock, flags);
            return &node_pool[i];
        }
    }
    spin_unlock_irqrestore(&pool_lock, flags);
    return NULL;
}

/* ================================================================
   Tree Manipulation
   ================================================================ */

static struct reg_node *find_or_create_child(struct reg_node *parent,
                                              const char *name,
                                              uint8_t rights) {
    struct reg_node *c = parent->first_child;
    while (c) {
        if (strcmp(c->name, name) == 0)
            return c;
        c = c->next_sibling;
    }
    struct reg_node *n = alloc_node();
    if (!n) return NULL;
    strncpy(n->name, name, REG_NAME_MAX - 1);
    n->parent       = parent;
    n->rights       = rights;
    n->next_sibling = parent->first_child;
    parent->first_child = n;
    return n;
}

struct reg_node *reg_mkpath(const char *path, uint8_t rights) {
    if (!path || !reg_root) return NULL;

    char buf[REG_PATH_MAX];
    strncpy(buf, path, REG_PATH_MAX - 1);
    buf[REG_PATH_MAX - 1] = '\0';

    const char *p = buf;
    if (*p == '/') p++;

    struct reg_node *cur = reg_root;
    char token[REG_NAME_MAX];

    while (*p) {
        int len = 0;
        while (*p && *p != '/' && len < REG_NAME_MAX - 1)
            token[len++] = *p++;
        token[len] = '\0';
        if (*p == '/') p++;
        if (len == 0) continue;

        cur = find_or_create_child(cur, token, rights);
        if (!cur) return NULL;
    }
    return cur;
}

struct reg_node *reg_lookup(const char *path) {
    if (!path || !reg_root) return NULL;
    if (path[0] == '\0' || strcmp(path, "/") == 0)
        return reg_root;

    char buf[REG_PATH_MAX];
    strncpy(buf, path, REG_PATH_MAX - 1);
    buf[REG_PATH_MAX - 1] = '\0';

    const char *p = buf;
    if (*p == '/') p++;

    struct reg_node *cur = reg_root;
    char token[REG_NAME_MAX];

    while (*p && cur) {
        int len = 0;
        while (*p && *p != '/' && len < REG_NAME_MAX - 1)
            token[len++] = *p++;
        token[len] = '\0';
        if (*p == '/') p++;
        if (len == 0) continue;

        struct reg_node *child = cur->first_child;
        cur = NULL;
        while (child) {
            if (strcmp(child->name, token) == 0) {
                cur = child;
                break;
            }
            child = child->next_sibling;
        }
    }
    return cur;
}

/* ================================================================
   Public Key-Value API
   ================================================================ */

void registry_init(void) {
    memset(node_pool, 0, sizeof(node_pool));
    memset(pool_used, 0, sizeof(pool_used));

    reg_root = alloc_node();
    strncpy(reg_root->name, "/", REG_NAME_MAX - 1);
    reg_root->rights = RK_ALL;

    registry_set("system/hostname",    "NeXs");
    registry_set("system/version",     "0.2.0-micro");
    registry_set("theme/color",        "dark");
    registry_set("mouse/sensitivity",  "1.0");

    pr_info("Registry: Hierarchical registry initialized (%d node pool)\n",
            REG_POOL_SIZE);
}

int registry_set(const char *key, const char *value) {
    if (!key || !value) return -1;

    uint64_t flags;
    spin_lock_irqsave(&tree_lock, &flags);

    struct reg_node *n = reg_lookup(key);
    if (!n) n = reg_mkpath(key, RK_ALL);
    if (!n) { spin_unlock_irqrestore(&tree_lock, flags); return -1; }

    strncpy(n->value, value, REG_VAL_MAX - 1);
    n->value[REG_VAL_MAX - 1] = '\0';

    spin_unlock_irqrestore(&tree_lock, flags);
    return 0;
}

int registry_get(const char *key, char *buf, size_t size) {
    if (!key || !buf || size == 0) return -1;

    uint64_t flags;
    spin_lock_irqsave(&tree_lock, &flags);

    struct reg_node *n = reg_lookup(key);
    if (!n) { spin_unlock_irqrestore(&tree_lock, flags); return -1; }

    strncpy(buf, n->value, size - 1);
    buf[size - 1] = '\0';

    spin_unlock_irqrestore(&tree_lock, flags);
    return 0;
}

int registry_list(const char *path, char *buf, size_t size) {
    if (!buf || size == 0) return -1;

    struct reg_node *n = reg_lookup(path ? path : "/");
    if (!n) return -1;

    size_t off = 0;
    struct reg_node *child = n->first_child;
    while (child && off + 1 < size) {
        size_t len = strlen(child->name);
        if (off + len + 2 > size) break;
        memcpy(buf + off, child->name, len);
        off += len;
        buf[off++] = '\n';
        child = child->next_sibling;
    }
    buf[off] = '\0';
    return (int)off;
}

/* ================================================================
   IPC Queue API (non-blocking ring buffers per node)
   ================================================================ */

int reg_ipc_init_queue(const char *path) {
    struct reg_node *n = reg_lookup(path);
    if (!n) n = reg_mkpath(path, RK_ALL);
    if (!n) return -1;
    if (n->queue) return 0;

    struct reg_queue *q = (struct reg_queue *)kmalloc(sizeof(struct reg_queue));
    if (!q) return -1;

    memset(q, 0, sizeof(struct reg_queue));
    spin_lock_init(&q->lock);
    n->queue = q;
    return 0;
}

int reg_ipc_send(const char *path, const struct reg_msg *msg) {
    struct reg_node *n = reg_lookup(path);
    if (!n) return -1;
    if (!n->queue && reg_ipc_init_queue(path) != 0) return -1;
    if (!n->queue) return -1;

    struct reg_queue *q = n->queue;
    uint64_t flags;
    spin_lock_irqsave(&q->lock, &flags);

    if (q->count >= REG_QUEUE_DEPTH) {
        spin_unlock_irqrestore(&q->lock, flags);
        return -1;
    }

    q->ring[q->tail] = *msg;
    q->tail = (q->tail + 1) % REG_QUEUE_DEPTH;
    q->count++;

    spin_unlock_irqrestore(&q->lock, flags);
    return 0;
}

int reg_ipc_recv(const char *path, struct reg_msg *out) {
    struct reg_node *n = reg_lookup(path);
    if (!n || !n->queue) return -1;

    struct reg_queue *q = n->queue;
    uint64_t flags;
    spin_lock_irqsave(&q->lock, &flags);

    if (q->count == 0) {
        spin_unlock_irqrestore(&q->lock, flags);
        return -1;
    }

    *out = q->ring[q->head];
    q->head = (q->head + 1) % REG_QUEUE_DEPTH;
    q->count--;

    spin_unlock_irqrestore(&q->lock, flags);
    return 0;
}

int reg_ipc_pending(const char *path) {
    struct reg_node *n = reg_lookup(path);
    if (!n || !n->queue) return 0;
    return n->queue->count;
}

/* ================================================================
   Syscall implementations
   ================================================================ */

#define REG_OP_READ  0
#define REG_OP_WRITE 1
#define REG_OP_LIST  2

long sys_registry(int op, const char *key, char *value, size_t size) {
    char k_key[REG_PATH_MAX];
    char k_val[REG_VAL_MAX];

    if (vmm_copy_string_from_user(k_key, key, REG_PATH_MAX) != 0)
        return -1;

    if (op == REG_OP_WRITE) {
        if (vmm_copy_string_from_user(k_val, value, REG_VAL_MAX) != 0)
            return -1;
        return registry_set(k_key, k_val);

    } else if (op == REG_OP_READ) {
        if (registry_get(k_key, k_val, sizeof(k_val)) != 0)
            return -1;
        size_t copy_len = strlen(k_val) + 1;
        if (copy_len > size) copy_len = size;
        return vmm_copy_to_user(value, k_val, copy_len);

    } else if (op == REG_OP_LIST) {
        size_t alloc_sz = (size < 4096) ? size : 4096;
        char *tmp = (char *)kmalloc(alloc_sz);
        if (!tmp) return -1;
        int ret = registry_list(k_key, tmp, alloc_sz);
        if (ret >= 0) vmm_copy_to_user(value, tmp, (size_t)(ret + 1));
        kfree(tmp);
        return ret;
    }
    return -2;
}

long sys_reg_ipc_send(const char *path, const struct reg_msg *user_msg) {
    char k_path[REG_PATH_MAX];
    struct reg_msg msg;
    if (vmm_copy_string_from_user(k_path, path, REG_PATH_MAX) != 0) return -1;
    if (vmm_copy_from_user(&msg, user_msg, sizeof(msg)) != 0) return -1;
    msg.from = (int32_t)current_process->pid;
    return reg_ipc_send(k_path, &msg);
}

long sys_reg_ipc_recv(const char *path, struct reg_msg *user_msg) {
    char k_path[REG_PATH_MAX];
    struct reg_msg msg;
    if (vmm_copy_string_from_user(k_path, path, REG_PATH_MAX) != 0) return -1;
    if (reg_ipc_recv(k_path, &msg) != 0) return -1;
    return vmm_copy_to_user(user_msg, &msg, sizeof(msg));
}

long sys_reg_ipc_pending(const char *path) {
    char k_path[REG_PATH_MAX];
    if (vmm_copy_string_from_user(k_path, path, REG_PATH_MAX) != 0) return -1;
    return (long)reg_ipc_pending(k_path);
}

long sys_reg_list(const char *path, char *buf, size_t size) {
    char k_path[REG_PATH_MAX];
    if (vmm_copy_string_from_user(k_path, path, REG_PATH_MAX) != 0) return -1;
    size_t alloc_sz = (size < 4096) ? size : 4096;
    char *tmp = (char *)kmalloc(alloc_sz);
    if (!tmp) return -1;
    int ret = registry_list(k_path, tmp, alloc_sz);
    if (ret >= 0) vmm_copy_to_user(buf, tmp, (size_t)(ret + 1));
    kfree(tmp);
    return ret;
}
