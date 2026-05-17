# 🚀 OS1TEST-DEV: Modular Dual-Architecture Microkernel

[![License: GPL v2](https://img.shields.io/badge/License-GPL%20v2-blue.svg)](LICENSE)
[![Arch AArch64](https://img.shields.io/badge/arch-AArch64-green.svg)](https://developer.arm.com/architectures/learn-the-architecture/armv8-a-instruction-set-architecture)
[![Arch AMD64](https://img.shields.io/badge/arch-AMD64-red.svg)](https://en.wikipedia.org/wiki/X86-64)
[![Platform QEMU](https://img.shields.io/badge/platform-QEMU%20virt%2Fpc-orange.svg)](https://www.qemu.org/)
[![Language C99](https://img.shields.io/badge/language-C99-yellow.svg)](https://en.wikipedia.org/wiki/C99)

OS1TEST-DEV is a state-of-the-art, modular microkernel operating system built for both **AArch64 (ARM64)** and **AMD64 (x86_64)** architectures. It blends the design methodologies of **Plan 9** (everything is a file/registry node), **seL4** (thinned hardware abstraction layer and secure message passing), and **Mach4** (isolated asynchronous microkernel-resident subsystems).

Featuring virtual memory isolation, real-time GUI window compositor, preemptive round-robin multitasking, and an optimized zero-copy Ext4 read-only storage driver, OS1TEST-DEV provides a solid, highly modern reference for dual-architecture OS engineering.

---

## 🏛️ Architectural Framework

OS1TEST-DEV implements a strictly thinned **Three-Tier Architecture** that guarantees deep isolation, simple portability, and performance-centric graphic subsystems.

```
┌──────────────────────────────────────────────────────────┐
│                     USER SPACE (EL0)                     │
│                                                          │
│     [init] ──spawn──> [shell] ──────> [notification]     │
│       │                 │                     │          │
│       └─────────┬───────┴──────────┬──────────┘          │
└─────────────────┼──────────────────┼─────────────────────┘
                  │ IPC (Registry)   │ Syscalls
┌─────────────────▼──────────────────▼─────────────────────┐
│                    KERNEL CORE (EL1)                     │
│                                                          │
│    ┌──────────────┐ ┌───────────────┐ ┌──────────────┐   │
│    │  Scheduler   │ │  Compositor   │ │   VFS/Ext4   │   │
│    │ (Round-Robin)│ │(Overlap/Alpha)│ │(LRU Cache/GPT)   │   │
│    └──────┬───────┘ └───────┬───────┘ └──────┬───────┘   │
└───────────┼─────────────────┼────────────────┼───────────┘
            │ Abstraction     │ Abstraction    │ Abstraction
┌───────────▼─────────────────▼────────────────▼───────────┐
│                HAL (Platform Layer / Drivers)            │
│                                                          │
│    ┌──────────────┐ ┌───────────────┐ ┌──────────────┐   │
│    │   AArch64    │ │     AMD64     │ │   Drivers    │   │
│    │(MMU, Vectors)│ │(GDT,IDT,APIC) │ │(VirtIO, GIC) │   │
│    └──────────────┘ └───────────────┘ └──────────────┘   │
└──────────────────────────────────────────────────────────┘
```

### 1. Thinned HAL (Hardware Abstraction Layer)
To maximize security and clean up platform isolation, all high-level logic is moved out of architecture directories into the unified kernel core. The HAL is limited strictly to assembly vector traps, interrupt controls (`cli`/`sti`, `cpsid`/`cpsie`), architecture context structures (`pt_regs`), and direct MMIO character UART routines.

### 2. Unified Kernel Core
The main OS1 microkernel coordinates:
*   **Virtual Memory isolation**: Thread-safe virtual page mapping and translation table generation.
*   **Preemptive Task Scheduling**: Preemption driven by clock ticks via GICv2 on AArch64 and PIT on AMD64.
*   **Resident VFS & Graphics Compositor**: Kept inside the kernel core to ensure ultra-high-speed alpha blending, hardware-accelerated double buffering, and zero-copy Ext4 partition traversal.

### 3. Plan 9 Style Registry System
Hardcoded magic memory addresses, interrupt vectors, and hardware constants are strictly prohibited in the core logic. At boot, the bootloader or Flat Device Tree (FDT) parser registers all peripherals under the dynamic hierarchical key-value registry `/sys/registry`. Subsystems query registry nodes dynamically to fetch configuration attributes.

---

## ✨ Features

*   **Physical & Virtual Memory Managers (PMM/VMM)**:
    *   Dual-platform page directory builders (4-level paging on both architectures).
    *   Zone-based page frames allocator (DMA & Normal zones).
    *   Identity-mapped high kernel memory mapping (`TTBR1_EL1` on AArch64, high PML4 entry on AMD64).
*   **Process & Isolation Layer**:
    *   ELF64 execution segments parser mapping code, data, and BSS pages securely.
    *   Preemptive multi-process round-robin scheduler with a persistent idle task per CPU core.
    *   Dedicated architecture userland assembly context switch routines.
*   **Storage & Partitioning**:
    *   Zero-copy Ext4 read-only filesystem parser featuring indirect block parsing (supporting files up to 4MB).
    *   High-performance VirtIO-Block disk driver.
    *   GPT partition parsing with automatic fallback to MBR partition tables (ensures hybrid ISO boot support).
*   **Rich Graphic Subsystem**:
    *   Window compositor supporting overlapping, Z-order layout rendering, drag-and-drop focus, and alpha blending transparency.
    *   Double-buffered VirtIO-GPU framebuffer controller.
    *   Fixed-point 3D software rasterizer (cube perspective projection without floating-point requirements).
    *   TrueType (TTF) font translator utility mapping fonts into high-fidelity `.off` files.

---

## 💻 System Setup & Requirements

### 1. Compile Environment
*   **For AArch64 target**: `aarch64-linux-gnu-gcc` (Ubuntu/Debian) or `aarch64-elf-gcc` (macOS Homebrew).
*   **For AMD64 target**: Standard host compiler `gcc` (if x86_64 host) or `x86_64-elf-gcc` cross-compiler.
*   **Utilities**: GNU Make, `qemu-system-aarch64`, `qemu-system-x86_64`, `mtools`, `xorriso` (for ISO creation).

### 2. Quick Install Guide

#### Debian / Ubuntu:
```bash
sudo apt update
sudo apt install gcc-aarch64-linux-gnu gcc-x86-64-linux-gnu \
                 qemu-system-arm qemu-system-x86 mtools xorriso make git
```

#### macOS (Homebrew):
```bash
brew tap posixguy/cross-compilers
brew install aarch64-elf-gcc x86_64-elf-gcc qemu mtools xorriso make
```

---

## 🛠️ Build and Execution Guides

The build system utilizes a modular Makefile supporting the `ARCH` variable to target platforms.

### 1. Target AArch64 (ARM64)

#### Compile:
```bash
make clean ARCH=aarch64
make all ARCH=aarch64
```
This builds `kernel.bin`, `bootloader.bin`, and packages the user apps (`init`, `shell`, `notification_server`, `counter`) into the virtual disk structure inside `build/aarch64/`.

#### Run:
```bash
qemu-system-aarch64 \
  -machine virt \
  -cpu cortex-a57 \
  -m 1G \
  -kernel build/aarch64/kernel.bin \
  -drive file=disk.img,if=none,id=hd0,format=raw \
  -device virtio-blk-device,drive=hd0 \
  -device virtio-gpu-pci \
  -serial stdio
```

---

### 2. Target AMD64 (x86_64)

#### Compile:
```bash
make clean ARCH=amd64
make all ARCH=amd64
```
This builds the boot structures, kernel executable, and bundles them into `build/amd64/os.iso` using the GRUB stage loader configuration.

#### Run:
```bash
qemu-system-x86_64 \
  -m 1G \
  -cdrom build/amd64/os.iso \
  -device virtio-blk-pci,drive=hd0 \
  -drive file=disk.img,if=none,id=hd0,format=raw \
  -device virtio-gpu-pci \
  -serial stdio
```

---

## 📂 Codebase Architecture Mapping

Refer to [MANIFEST.md](file:///Users/olmo/Documents/git/ostest1/os1test-dev/MANIFEST.md) for a detailed, high-fidelity mapping of all source files in the repository.

---

## 🤝 Contribution Guidelines

*   **Indentation & Formatting**: Strictly follow the K&R style guide with 4-space indentation.
*   **Paging/Core Isolation**: Do not call architecture functions outside `/kernel/hal/`. All interaction between the Unified Core and platform operations must pass through `/kernel/core/include/core/hal.h` interface pointers.
*   **Paging Protection**: Never allocate user pages without setting the `USER` supervisor bit.

---

## ⚖️ License & Inspirations

This project is licensed under the **GNU General Public License, Version 2 (GPLv2)** - see the [LICENSE](LICENSE) file for details. This open-source license aligns the project directly with the licensing philosophy of **Linux**.

### 🌟 Reference Implementations & Inspirations

The architecture of OS1 has been designed by drawing inspiration from major operating system architectures. Plan 9 and seL4 represent the primary pillars of our technical design, followed by Linux and BSD models. Below is the mapping of our key architectural concepts to their reference sources in precise priority order:

1.  **Plan 9 from Bell Labs (Primary Pillars)**:
    *   *Inspiration*: "Everything is a file/resource" philosophy, hierarchical dynamically mounted key-value trees, and native ring buffers for IPC synchronization.
    *   *File/Code Reference*: The hierarchical dynamic registry keys and ring-buffer serialization mapped in [registry.c](file:///Users/olmo/Documents/git/ostest1/os1test-dev/kernel/libkernel/src/registry.c). Plan 9 style system call wrappers (`rfork`, `pread`, `pwrite`, `await`) planned in user libraries.
2.  **seL4 (Secure Embedded L4 - Primary Pillars)**:
    *   *Inspiration*: Strictly thinned Hardware Abstraction Layer (HAL) focused solely on assembly context setups, exception routing, and MMU directory table loads.
    *   *File/Code Reference*: Assembly entry boundaries in [exception.S](file:///Users/olmo/Documents/git/ostest1/os1test-dev/kernel/hal/arch/aarch64/cpu/exception.S) (AArch64) and [start.S](file:///Users/olmo/Documents/git/ostest1/os1test-dev/kernel/hal/arch/amd64/boot/start.S) (AMD64), context state mapping in `pt_regs`.
3.  **Linux (Kernel)**:
    *   *Inspiration*: Intrusive circular double-linked list structures, K&R style code conventions, and robust Ext4 file traversal logic.
    *   *File/Code Reference*: Double-linked list utility in [list.h](file:///Users/olmo/Documents/git/ostest1/os1test-dev/kernel/core/include/core/list.h), storage block parsing in [ext4.c](file:///Users/olmo/Documents/git/ostest1/os1test-dev/kernel/core/src/fs/ext4.c) and partition structures in [gpt.c](file:///Users/olmo/Documents/git/ostest1/os1test-dev/kernel/core/src/fs/gpt.c).
4.  **base-nexs Project**:
    *   *Inspiration*: Unified system service mapping paradigms and registry loop protocols.
    *   *File/Code Reference*: Architecture registry logic under [registry.c](file:///Users/olmo/Documents/git/ostest1/os1test-dev/kernel/libkernel/src/registry.c) and dynamic service coordination.
5.  **BSD / FreeBSD (VFS Layer)**:
    *   *Inspiration*: BSD-style Virtual File System (VFS) mounting mechanism, file node (vnode) virtualization, and path lookup utilities (`namei`, `nameidata`).
    *   *File/Code Reference*: Mount and vnode interface representations planned under resident filesystem management ([vfs.h](file:///Users/olmo/Documents/git/ostest1/os1test-dev/kernel/core/include/core/vfs.h)).
6.  **Mach4 (Mach Microkernel)**:
    *   *Inspiration*: Fully isolated helper servers communicating with the core through port-based IPC pipelines and asynchronous scheduling.
    *   *File/Code Reference*: IPC dispatch and IPC registry message queues (`SYS_REG_IPC_SEND`/`SYS_REG_IPC_RECV`) implemented under [syscall.c](file:///Users/olmo/Documents/git/ostest1/os1test-dev/kernel/core/src/syscall.c).
7.  **Font Rasterization Libraries (stb_truetype & stb_easy_font)**:
    *   *Inspiration*: Standalone, header-only lightweight graphics typography engine by Sean Barrett.
    *   *File/Code Reference*: TTF parsing tools in [stb_truetype.h](file:///Users/olmo/Documents/git/ostest1/os1test-dev/tools/stb_truetype.h) and user fonts output in [stb_easy_font.h](file:///Users/olmo/Documents/git/ostest1/os1test-dev/user/sys/include/stb_easy_font.h).
8.  **DoomGeneric Engine**:
    *   *Inspiration*: Standardized framework for quick application and game porting on customized embedded framebuffers.
    *   *File/Code Reference*: Custom blitter pipelines and input handlers integrated within user graphic applications.
9.  **Limine Bootloader**:
    *   *Inspiration*: Bootloader stage configurations and boot tags passing, ELF segments unpacking boundaries.
    *   *File/Code Reference*: Multi-stage assembly setups and stage loaders inside [kernel/hal/boot/](file:///Users/olmo/Documents/git/ostest1/os1test-dev/kernel/hal/boot/).
