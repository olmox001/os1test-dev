#ifndef _DRIVERS_VIRTIO_HAL_H
#define _DRIVERS_VIRTIO_HAL_H

#include <kernel/types.h>
#include <arch/arch.h>

/* VirtIO Transport Type */
typedef enum {
    VIRTIO_TRANSPORT_MMIO,
    VIRTIO_TRANSPORT_PCI_IO
} virtio_transport_t;

typedef struct {
    virtio_transport_t transport;
    uint64_t base;
} virtio_device_t;

static inline uint32_t virtio_read32(virtio_device_t *dev, uint32_t offset) {
    if (dev->transport == VIRTIO_TRANSPORT_MMIO) {
        return *(volatile uint32_t *)(dev->base + offset);
    } else {
        /* x86 Port I/O */
        return inl((uint16_t)dev->base + offset);
    }
}

static inline void virtio_write32(virtio_device_t *dev, uint32_t offset, uint32_t val) {
    if (dev->transport == VIRTIO_TRANSPORT_MMIO) {
        *(volatile uint32_t *)(dev->base + offset) = val;
    } else {
        /* x86 Port I/O */
        outl((uint16_t)dev->base + offset, val);
    }
}

#endif
