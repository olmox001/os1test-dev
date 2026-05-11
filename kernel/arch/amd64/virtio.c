#include <kernel/arch.h>
#include <kernel/printk.h>
#include <drivers/virtio.h>
#include <drivers/pci.h>

/* Flag to indicate a Modern (MMIO) base address in our internal HAL */
#define VIRTIO_BASE_MODERN_FLAG (1ULL << 62)

/* Translation from common MMIO offsets to Legacy PCI offsets (I/O Port) */
static uint32_t translate_legacy(uint32_t offset) {
    switch (offset) {
        case VIRTIO_MMIO_DEVICE_FEATURES: return 0x00;
        case VIRTIO_MMIO_DRIVER_FEATURES: return 0x04;
        case VIRTIO_MMIO_QUEUE_PFN:       return 0x08;
        case VIRTIO_MMIO_QUEUE_NUM:       return 0x0C;
        case VIRTIO_MMIO_QUEUE_SEL:       return 0x0E;
        case VIRTIO_MMIO_QUEUE_NOTIFY:    return 0x10;
        case VIRTIO_MMIO_STATUS:          return 0x12;
        case VIRTIO_MMIO_INTERRUPT_STATUS: return 0x13;
        case VIRTIO_MMIO_INTERRUPT_ACK:    return 0x13;
        default: return 0xFFFFFFFF;
    }
}

/* Translation from common MMIO offsets to Modern PCI Common Config offsets */
static uint32_t translate_modern(uint32_t offset) {
    switch (offset) {
        case VIRTIO_MMIO_DEVICE_FEATURES: return 0x04; // device_feature
        case VIRTIO_MMIO_DRIVER_FEATURES: return 0x0C; // driver_feature
        case VIRTIO_MMIO_QUEUE_SEL:       return 0x16; // queue_select
        case VIRTIO_MMIO_QUEUE_NUM:       return 0x18; // queue_size
        case VIRTIO_MMIO_STATUS:          return 0x14; // device_status
        case VIRTIO_MMIO_QUEUE_READY:     return 0x1C; // queue_enable
        case VIRTIO_MMIO_QUEUE_DESC_LOW:  return 0x20; // queue_desc_lo
        case VIRTIO_MMIO_QUEUE_DESC_HIGH: return 0x24; // queue_desc_hi
        case VIRTIO_MMIO_QUEUE_DRIVER_LOW: return 0x28; // queue_avail_lo
        case VIRTIO_MMIO_QUEUE_DRIVER_HIGH: return 0x2C; // queue_avail_hi
        case VIRTIO_MMIO_QUEUE_DEVICE_LOW: return 0x30; // queue_used_lo
        case VIRTIO_MMIO_QUEUE_DEVICE_HIGH: return 0x34; // queue_used_hi
        default: return 0xFFFFFFFF;
    }
}

uint32_t virtio_read_reg(uintptr_t base, uint32_t offset) {
    if (offset == VIRTIO_MMIO_MAGIC_VALUE) return 0x74726976;
    if (offset == VIRTIO_MMIO_VERSION) return (base & VIRTIO_BASE_MODERN_FLAG) ? 2 : 1;

    if (base & VIRTIO_BASE_MODERN_FLAG) {
        uintptr_t addr = base & ~VIRTIO_BASE_MODERN_FLAG;
        uint32_t mod_off = translate_modern(offset);
        if (mod_off == 0xFFFFFFFF) return 0;
        return *(volatile uint32_t *)(addr + mod_off);
    } else {
        uint32_t pci_off = translate_legacy(offset);
        if (pci_off == 0xFFFFFFFF) return 0;
        uint16_t port = (uint16_t)base + pci_off;
        if (pci_off == 0x12 || pci_off == 0x13) return inb(port);
        if (pci_off == 0x0C || pci_off == 0x0E || pci_off == 0x10) return inw(port);
        return inl(port);
    }
}

void virtio_write_reg(uintptr_t base, uint32_t offset, uint32_t val) {
    if (base & VIRTIO_BASE_MODERN_FLAG) {
        uintptr_t addr = base & ~VIRTIO_BASE_MODERN_FLAG;
        uint32_t mod_off = translate_modern(offset);
        if (mod_off != 0xFFFFFFFF) {
            *(volatile uint32_t *)(addr + mod_off) = val;
        }
    } else {
        uint32_t pci_off = translate_legacy(offset);
        if (pci_off == 0xFFFFFFFF) return;
        uint16_t port = (uint16_t)base + pci_off;
        if (pci_off == 0x12) outb(port, (uint8_t)val);
        else if (pci_off == 0x0C || pci_off == 0x0E || pci_off == 0x10) outw(port, (uint16_t)val);
        else outl(port, val);
    }
}

int arch_virtio_probe(uint32_t device_id, uintptr_t *out_base, uint32_t *out_irq) {
    for (int dev = 0; dev < 32; dev++) {
        uint32_t bdf = (0 << 16) | (dev << 8) | 0;
        uint32_t id = pci_config_read(0, dev, 0, 0);
        if (id == 0xFFFFFFFF) continue;

        uint16_t pci_devid = id >> 16;
        uint16_t virtio_id = 0;
        int is_modern = 0;

        if (pci_devid >= 0x1000 && pci_devid <= 0x103F) {
            virtio_id = pci_config_read(0, dev, 0, 0x2C) >> 16;
        } else if (pci_devid >= 0x1040 && pci_devid <= 0x107F) {
            virtio_id = pci_devid - 0x1040;
            is_modern = 1;
        }

        if (virtio_id == device_id) {
            pr_info("VirtIO: Match found! Device ID %d at PCI 00:%02x.0 (%s)\n", 
                    virtio_id, dev, is_modern ? "Modern" : "Legacy");

            if (is_modern) {
                /* Modern: Parse Capabilities (Type 0x09) */
                uint32_t status_reg = pci_config_read(0, dev, 0, 0x04);
                if (!(status_reg & (1 << 20))) {
                    pr_info("  Device has no capabilities list\n");
                }
                
                uint8_t cap_ptr = pci_config_read(0, dev, 0, 0x34) & 0xFC;
                while (cap_ptr >= 0x40) {
                    uint32_t cap_header = pci_config_read(0, dev, 0, cap_ptr);
                    uint8_t cap_id = cap_header & 0xFF;
                    
                    if (cap_id == 0x09) { /* Vendor Specific */
                        uint32_t cap_info = pci_config_read(0, dev, 0, cap_ptr + 3);
                        uint8_t cfg_type = cap_info & 0xFF;
                        if (cfg_type == 1) { /* Common Config */
                            uint8_t bar_idx = (cap_info >> 8) & 0xFF;
                            uint32_t offset = pci_config_read(0, dev, 0, cap_ptr + 8);
                            uint32_t bar_val = pci_get_bar(bdf, bar_idx);
                            
                            pr_info("  Common Config found in BAR%d at offset 0x%x\n", bar_idx, offset);
                            
                            if (!(bar_val & 1)) { /* MMIO BAR */
                                uintptr_t base = (bar_val & ~0xF) + offset;
                                if (out_base) *out_base = base | VIRTIO_BASE_MODERN_FLAG;
                                if (out_irq) *out_irq = 32 + pci_get_interrupt(bdf);
                                pr_info("  Base Address: 0x%lx (Modern)\n", base);
                                return 0;
                            }
                        }
                    }
                    cap_ptr = (cap_header >> 8) & 0xFC;
                }
            } else {
                /* Legacy: Look for I/O BAR (usually BAR0) */
                for (int bar = 0; bar < 6; bar++) {
                    uint32_t bar_val = pci_get_bar(bdf, bar);
                    if (bar_val & 1) {
                        uintptr_t base = bar_val & ~3;
                        if (out_base) *out_base = base;
                        if (out_irq) *out_irq = 32 + pci_get_interrupt(bdf);
                        pr_info("  Base Address: 0x%lx (Legacy I/O)\n", base);
                        return 0;
                    }
                }
            }
        }
    }
    return -1;
}
