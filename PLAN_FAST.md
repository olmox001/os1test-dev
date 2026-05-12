# PLAN_FAST.md - OS1TEST Evolution Plan

## Project Status Overview

- **Architecture**: Dual-arch Microkernel (AArch64 & AMD64).
- **Core format**: ELF (non-converted to bin where possible to preserve structure).
- **Memory**: PMM with zone support (DMA/Normal), VMM with 4-level tables.
- **Drivers**: Multi-platform VirtIO support in progress.
- **Boot**: Custom bootloader + QEMU `-kernel` support.

## Phase 1: Infrastructure & Log Filtering
> **Goal**: Improve debuggability by focusing on errors/warnings and finalize project documentation.
- [ ] Modify `kernel/lib/printk.c` to set `console_loglevel = KERN_WARNING`.
- [ ] Create `PLAN_FAST.md` (this document) and verify system state.
- [ ] **Verification**: `make run ARCH=aarch64` should show significantly fewer `[INFO]` logs.

## Phase 2: HAL Abstraction & Unified Headers
> **Goal**: Unify `arch.h` so that drivers and core code don't need arch-specific `#ifdef`s.
- [ ] Refactor `kernel/include/kernel/arch.h` to use a generic naming convention for all hardware primitives.
- [ ] Ensure `__arch_*` functions in `kernel/arch/*/include/arch/arch.h` perfectly map to the generic interface.
- [ ] Implement generic `arch_virtio_read/write` that abstracts MMIO (ARM) vs Port I/O (x86).
- [ ] **Verification**: Successful compilation and boot for both architectures.

## Phase 3: Automatic Hardware Discovery (RAM & CPUs)
> **Goal**: Remove hardcoded memory limits and CPU counts.
- [ ] **AArch64**: Implement a lightweight FDT (Device Tree) parser in `kernel/lib/fdt.c`.
- [ ] **AArch64**: Use FDT to detect RAM regions and initialize PMM accordingly.
- [ ] **AMD64**: Implement Multiboot2 memory map parsing in `platform.c`.
- [ ] **Verification**: `make run` logs should show detected RAM matching QEMU `-m` parameter.

## Phase 4: Driver Refactoring (PCI vs VirtIO)
> **Goal**: Enable VirtIO drivers to work on both PCI (x86) and MMIO (ARM).
- [ ] Implement a unified `virtio_bus` abstraction.
- [ ] Refactor `virtio_blk`, `virtio_gpu`, and `virtio_input` to use the bus abstraction.
- [ ] Implement PCI scanning on AMD64 to find VirtIO devices.
- [ ] **Verification**: Block and GPU drivers working on both platforms using the same core code.

## Phase 5: ISO Boot & ELF Support
> **Goal**: Support standard ISO boot without losing ELF structure.
- [ ] Update `Makefile` to create an ISO for AArch64 using `grub-mkrescue` or `xorriso`.
- [ ] Ensure the bootloader can load the `kernel.elf` directly from the ISO (preservando l'ELF).
- [ ] Update `make run` to boot from the generated ISO.
- [ ] **Verification**: `make run` boots from CD-ROM device in QEMU.
