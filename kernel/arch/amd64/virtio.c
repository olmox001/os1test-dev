#include <kernel/arch.h>
#include <drivers/virtio.h>
#include <drivers/pci.h>

/* Translation from MMIO offsets to Legacy PCI offsets */
static uint32_t translate_offset(uint32_t offset) {
    switch (offset) {
        case VIRTIO_MMIO_DEVICE_FEATURES: return 0x00;
        case VIRTIO_MMIO_DRIVER_FEATURES: return 0x04;
        case VIRTIO_MMIO_QUEUE_PFN:       return 0x08;
        case VIRTIO_MMIO_QUEUE_NUM:       return 0x0C;
        case VIRTIO_MMIO_QUEUE_SEL:       return 0x0E;
        case VIRTIO_MMIO_QUEUE_NOTIFY:    return 0x10;
        case VIRTIO_MMIO_STATUS:          return 0x12;
        case VIRTIO_MMIO_INTERRUPT_STATUS: return 0x13;
        case VIRTIO_MMIO_INTERRUPT_ACK:    return 0x13; // Reading ISR status acks it in legacy
        default: return 0xFFFFFFFF;
    }
}

uint32_t arch_virtio_read32(uintptr_t base, uint32_t offset) {
    if (offset == VIRTIO_MMIO_MAGIC_VALUE) return 0x74726976;
    if (offset == VIRTIO_MMIO_VERSION) return 1;
    if (offset == VIRTIO_MMIO_DEVICE_ID) return 2; // Fixed for now, or fetch from PCI ID

    uint32_t pci_off = translate_offset(offset);
    if (pci_off == 0xFFFFFFFF) return 0;

    if (pci_off == 0x12 || pci_off == 0x13) return inb((uint16_t)base + pci_off);
    if (pci_off == 0x0C || pci_off == 0x0E || pci_off == 0x10) return inw((uint16_t)base + pci_off);
    return inl((uint16_t)base + pci_off);
}

void arch_virtio_write32(uintptr_t base, uint32_t offset, uint32_t val) {
    uint32_t pci_off = translate_offset(offset);
    if (pci_off == 0xFFFFFFFF) return;

    if (pci_off == 0x12) outb((uint16_t)base + pci_off, (uint8_t)val);
    else if (pci_off == 0x0C || pci_off == 0x0E || pci_off == 0x10) outw((uint16_t)base + pci_off, (uint16_t)val);
    else outl((uint16_t)base + pci_off, val);
}

int arch_virtio_probe(uint32_t device_id, uintptr_t *out_base, uint32_t *out_irq) {
    /* Map VirtIO Device ID to PCI Device ID (Legacy) */
    uint16_t pci_devid = 0;
    if (device_id == VIRTIO_DEV_BLOCK) pci_devid = 0x1001;
    else if (device_id == VIRTIO_DEV_GPU) pci_devid = 0x1010;
    else if (device_id == VIRTIO_DEV_INPUT) pci_devid = 0x1012;

    int bdf = pci_find_device(0x1AF4, pci_devid);
    if (bdf != -1) {
        uint32_t bar0 = pci_get_bar(bdf, 0);
        if (bar0 & 1) { // I/O BAR
            if (out_base) *out_base = bar0 & ~3;
            if (out_irq) *out_irq = 32 + pci_get_interrupt(bdf);
            return 0;
        }
    }
    return -1;
}
