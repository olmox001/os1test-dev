/*
 * kernel/include/drivers/virtio_blk.h
 * VirtIO Block Device Driver
 */
#ifndef _DRIVERS_VIRTIO_BLK_H
#define _DRIVERS_VIRTIO_BLK_H

#include <drivers/virtio.h>
#include <kernel/types.h>

/* Feature bits */
#define VIRTIO_BLK_F_SIZE_MAX 1
#define VIRTIO_BLK_F_SEG_MAX 2
#define VIRTIO_BLK_F_GEOMETRY 4
#define VIRTIO_BLK_F_RO 5
#define VIRTIO_BLK_F_BLK_SIZE 6
#define VIRTIO_BLK_F_FLUSH 9
#define VIRTIO_BLK_F_TOPOLOGY 10
#define VIRTIO_BLK_F_CONFIG_WCE 11

/* Request types */
#define VIRTIO_BLK_T_IN 0
#define VIRTIO_BLK_T_OUT 1
#define VIRTIO_BLK_T_FLUSH 4

/* Status */
#define VIRTIO_BLK_S_OK 0
#define VIRTIO_BLK_S_IOERR 1
#define VIRTIO_BLK_S_UNSUPP 2

struct virtio_blk_req {
  uint32_t type;
  uint32_t reserved;
  uint64_t sector;
};

/* API */
void virtio_blk_init(void);
int virtio_blk_read(void *buf, uint64_t sector, uint32_t count);
int virtio_blk_write(void *buf, uint64_t sector, uint32_t count);

#endif /* _DRIVERS_VIRTIO_BLK_H */
