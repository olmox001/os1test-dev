#ifndef _KERNEL_REGISTRY_H
#define _KERNEL_REGISTRY_H

#include <libkernel/types.h>
#include <core/spinlock.h>
#include <core/ipc.h>   /* provides struct reg_msg */

#define REG_NAME_MAX    64
#define REG_PATH_MAX    256
#define REG_VAL_MAX     128
#define REG_POOL_SIZE   256
#define REG_QUEUE_DEPTH 16

#define RK_READ  1
#define RK_WRITE 2
#define RK_ALL   (RK_READ | RK_WRITE)

/* Per-key non-blocking IPC ring buffer */
struct reg_queue {
    struct reg_msg ring[REG_QUEUE_DEPTH];
    int            head, tail, count;
    spinlock_t     lock;
};

/* Hierarchical tree node — allocated from a static pool */
struct reg_node {
    char              name[REG_NAME_MAX];
    char              value[REG_VAL_MAX];
    struct reg_node  *parent;
    struct reg_node  *first_child;
    struct reg_node  *next_sibling;
    struct reg_queue *queue;
    uint8_t           rights;
};

/* ---------------------------------------------------------------
   Core tree API
   --------------------------------------------------------------- */
void             registry_init(void);
struct reg_node *reg_mkpath(const char *path, uint8_t rights);
struct reg_node *reg_lookup(const char *path);
int              registry_set(const char *key, const char *value);
int              registry_get(const char *key, char *buf, size_t size);
int              registry_list(const char *path, char *buf, size_t size);

/* ---------------------------------------------------------------
   Per-key IPC queue API
   --------------------------------------------------------------- */
int reg_ipc_init_queue(const char *path);
int reg_ipc_send(const char *path, const struct reg_msg *msg);
int reg_ipc_recv(const char *path, struct reg_msg *out);
int reg_ipc_pending(const char *path);

/* ---------------------------------------------------------------
   Syscall implementations (called from syscall dispatcher)
   --------------------------------------------------------------- */
long sys_registry(int op, const char *key, char *value, size_t size);
long sys_reg_ipc_send(const char *path, const struct reg_msg *user_msg);
long sys_reg_ipc_recv(const char *path, struct reg_msg *user_msg);
long sys_reg_ipc_pending(const char *path);
long sys_reg_list(const char *path, char *buf, size_t size);

#endif /* _KERNEL_REGISTRY_H */
