#include <kernel/arch.h>
#include <drivers/virtio.h>

uint32_t arch_virtio_read32(uintptr_t base, uint32_t offset) {
    return *(volatile uint32_t *)(base + offset);
}

void arch_virtio_write32(uintptr_t base, uint32_t offset, uint32_t val) {
    *(volatile uint32_t *)(base + offset) = val;
}

int arch_virtio_probe(uint32_t device_id, uintptr_t *out_base, uint32_t *out_irq) {
    for (int i = 0; i < VIRTIO_COUNT; i++) {
        uintptr_t base = VIRTIO_MMIO_BASE + i * VIRTIO_MMIO_STRIDE;
        if (arch_virtio_read32(base, VIRTIO_MMIO_MAGIC_VALUE) == 0x74726976) {
            if (arch_virtio_read32(base, VIRTIO_MMIO_DEVICE_ID) == device_id) {
                if (out_base) *out_base = base;
                if (out_irq) *out_irq = 48 + i; // QEMU virt machine IRQs start at 32 + 16
                return 0;
            }
        }
    }
    return -1;
}
