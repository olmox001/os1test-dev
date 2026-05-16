#include <drivers/pci.h>
#include <arch/arch.h>

uint32_t arch_pci_config_read(uint8_t bus, uint8_t device, uint8_t func, uint8_t offset) {
    uint32_t address = (uint32_t)((uint32_t)bus << 16) | 
                       (uint32_t)((uint32_t)device << 11) |
                       (uint32_t)((uint32_t)func << 8) | 
                       (offset & 0xFC) | 
                       ((uint32_t)0x80000000);
    outl(0xCF8, address);
    return inl(0xCFC);
}

void arch_pci_config_write(uint8_t bus, uint8_t device, uint8_t func, uint8_t offset, uint32_t value) {
    uint32_t address = (uint32_t)((uint32_t)bus << 16) | 
                       (uint32_t)((uint32_t)device << 11) |
                       (uint32_t)((uint32_t)func << 8) | 
                       (offset & 0xFC) | 
                       ((uint32_t)0x80000000);
    outl(0xCF8, address);
    outl(0xCFC, value);
}
