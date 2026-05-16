#include <drivers/pci.h>
#include <kernel/types.h>

/* QEMU virt machine ECAM base for PCI */
#define PCI_ECAM_BASE 0x3f000000UL

uint32_t arch_pci_config_read(uint8_t bus, uint8_t device, uint8_t func, uint8_t offset) {
    /* ECAM Address: Base + (Bus << 20) | (Dev << 15) | (Func << 12) | Offset */
    uintptr_t addr = PCI_ECAM_BASE | 
                     ((uintptr_t)bus << 20) | 
                     ((uintptr_t)device << 15) | 
                     ((uintptr_t)func << 12) | 
                     (offset & 0xFFF);
    
    return *(volatile uint32_t *)addr;
}

void arch_pci_config_write(uint8_t bus, uint8_t device, uint8_t func, uint8_t offset, uint32_t value) {
    uintptr_t addr = PCI_ECAM_BASE | 
                     ((uintptr_t)bus << 20) | 
                     ((uintptr_t)device << 15) | 
                     ((uintptr_t)func << 12) | 
                     (offset & 0xFFF);
    
    *(volatile uint32_t *)addr = value;
}
