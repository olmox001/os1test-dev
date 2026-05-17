# Manifest: OS1 Project Structure

This manifest documents the complete, clean directory structure of the **OS1 Microkernel** repository following the Phase 1 HAL relocation and architectural thinning.

---

## 📂 Directory Layout Overview

```
.
├── LICENSE                      # GNU General Public License v2 (GPLv2)
├── Makefile                     # Central build system (AArch64 / AMD64)
├── README.md                    # Dual-architecture developer documentation
├── REFACTOR_PLAN.md             # Multi-phase refactoring roadmap
├── STATUS.md                    # Active implementation status
├── WORK_SUMMARY.md              # Historical development log
├── grub.cfg                     # GRUB configuration for AMD64 hybrid ISO boot
│
├── kernel/                      # OS1 Kernel Space
│   ├── core/                    # Unified Core (Microkernel Logic)
│   │   ├── include/core/        # Unified core header files
│   │   └── src/                 # Core implementation (VFS, Scheduler, Compositor)
│   │
│   ├── hal/                     # Hardware Abstraction Layer (Minimal Abstractions)
│   │   ├── arch/                # Platform-specific architectures
│   │   │   ├── aarch64/         # AArch64 specific assembly, MMU, exceptions
│   │   │   └── amd64/           # AMD64 specific GDT, IDT, APIC, context
│   │   ├── boot/                # Bootloaders & early stages
│   │   ├── drivers/             # Decoupled device drivers (VirtIO, UART, GIC, PIT)
│   │   └── user/                # Relocated userland startup and syscall wrappers
│   │
│   └── libkernel/               # Shared kernel and boot loader utility library
│       ├── include/libkernel/   # Common types, string, ctype, math headers
│       └── src/                 # Utility implementation (Registry tree, vsnprintf)
│
├── tools/                       # Host building tools (mkdisk, mkfont, ttf2off)
│
└── user/                        # Isolated User Space (EL0)
    ├── bin/                     # User-space utility applications (demo3d, counter)
    └── sys/                     # Critical system processes
        ├── bin/                 # Init process, notification server, shell, fontman
        ├── include/             # User standard standard headers (POSIX unistd, stdio)
        └── lib/                 # User libc core library (malloc, drawing, etc.)
```

---

## 🗂️ Detailed File Catalog

### 1. Unified Microkernel Core (`kernel/core/`)
*   **Headers** (`kernel/core/include/core/`):
    *   [syscall.h](file:///Users/olmo/Documents/git/ostest1/os1test-dev/kernel/core/include/core/syscall.h): System call numbers and interface declarations.
    *   [sched.h](file:///Users/olmo/Documents/git/ostest1/os1test-dev/kernel/core/include/core/sched.h): Scheduler structures and round-robin queues.
    *   [vfs.h](file:///Users/olmo/Documents/git/ostest1/os1test-dev/kernel/core/include/core/vfs.h): Resident VFS interfaces.
    *   [ext4.h](file:///Users/olmo/Documents/git/ostest1/os1test-dev/kernel/core/include/core/ext4.h): Ext4 filesystem mapping structures.
    *   [elf.h](file:///Users/olmo/Documents/git/ostest1/os1test-dev/kernel/core/include/core/elf.h): Kernel-only ELF segment parsing.
    *   [boot_fs.h](file:///Users/olmo/Documents/git/ostest1/os1test-dev/kernel/core/include/core/boot_fs.h): Simplified early boot filesystem loader.
*   **Sources** (`kernel/core/src/`):
    *   [main.c](file:///Users/olmo/Documents/git/ostest1/os1test-dev/kernel/core/src/main.c): Microkernel main entry point (arch-independent boot coordination).
    *   [cpu.c](file:///Users/olmo/Documents/git/ostest1/os1test-dev/kernel/core/src/cpu.c): Core-independent CPU loop states.
    *   [stubs.c](file:///Users/olmo/Documents/git/ostest1/os1test-dev/kernel/core/src/stubs.c): Unresolved VFS syscall stubs.
    *   [timer.c](file:///Users/olmo/Documents/git/ostest1/os1test-dev/kernel/core/src/timer.c): Logical timer management.
    *   [syscall_proc.c](file:///Users/olmo/Documents/git/ostest1/os1test-dev/kernel/core/src/syscall_proc.c): Process management syscall implementation.
    *   [syscall.c](file:///Users/olmo/Documents/git/ostest1/os1test-dev/kernel/core/src/syscall.c): Global system call multiplexer.
    *   [boot_fs.c](file:///Users/olmo/Documents/git/ostest1/os1test-dev/kernel/core/src/boot_fs.c): Multilevel indirect block loading and partition table parser.
    *   **Graphics & Filesystem Core**:
        *   [graphics/compositor.c](file:///Users/olmo/Documents/git/ostest1/os1test-dev/kernel/core/src/graphics/compositor.c): Real-time graphics blitter and overlap manager.
        *   [fs/ext4.c](file:///Users/olmo/Documents/git/ostest1/os1test-dev/kernel/core/src/fs/ext4.c): Standard Ext4 resident block parser.
        *   [fs/gpt.c](file:///Users/olmo/Documents/git/ostest1/os1test-dev/kernel/core/src/fs/gpt.c): GPT partition scanning.

### 2. Hardware Abstraction Layer (`kernel/hal/`)
*   **AArch64 Architecture** (`kernel/hal/arch/aarch64/`):
    *   [start.S](file:///Users/olmo/Documents/git/ostest1/os1test-dev/kernel/hal/arch/aarch64/boot/start.S): Startup bootstrap assembly and MMU enabling.
    *   [exception.S](file:///Users/olmo/Documents/git/ostest1/os1test-dev/kernel/hal/arch/aarch64/cpu/exception.S): Exception vector table and raw interrupt entries.
    *   [syscall.c](file:///Users/olmo/Documents/git/ostest1/os1test-dev/kernel/hal/arch/aarch64/cpu/syscall.c): Syscall parsing and register dumping for AArch64.
    *   [mmu.c](file:///Users/olmo/Documents/git/ostest1/os1test-dev/kernel/hal/arch/aarch64/mm/mmu.c): 4-level translation table mapping logic.
    *   [platform.c](file:///Users/olmo/Documents/git/ostest1/os1test-dev/kernel/hal/arch/aarch64/platform.c): Platform configuration and memory boundaries for QEMU Virt.
    *   [kernel.ld](file:///Users/olmo/Documents/git/ostest1/os1test-dev/kernel/hal/arch/aarch64/kernel.ld): AArch64 linker script.
*   **AMD64 Architecture** (`kernel/hal/arch/amd64/`):
    *   [start.S](file:///Users/olmo/Documents/git/ostest1/os1test-dev/kernel/hal/arch/amd64/boot/start.S): Multiboot v2 entry point and 64-bit long-mode transition.
    *   [cpu.c](file:///Users/olmo/Documents/git/ostest1/os1test-dev/kernel/hal/arch/amd64/cpu/cpu.c): CPU setup, GDT/IDT register mappings, and LAPIC initializing.
    *   [idt.c](file:///Users/olmo/Documents/git/ostest1/os1test-dev/kernel/hal/arch/amd64/cpu/idt.c): Gate descriptors and interrupt vectors.
    *   [isr_stubs.S](file:///Users/olmo/Documents/git/ostest1/os1test-dev/kernel/hal/arch/amd64/cpu/isr_stubs.S): Assembly interrupt service routine handlers.
    *   [syscall.S](file:///Users/olmo/Documents/git/ostest1/os1test-dev/kernel/hal/arch/amd64/cpu/syscall.S): Low-latency `syscall`/`sysret` handlers.
    *   [mmu.c](file:///Users/olmo/Documents/git/ostest1/os1test-dev/kernel/hal/arch/amd64/mm/mmu.c): AMD64 page directory (PML4/PDPT/PD/PT) mappings.
    *   [kernel.ld](file:///Users/olmo/Documents/git/ostest1/os1test-dev/kernel/hal/arch/amd64/kernel.ld): AMD64 linker script.
*   **Drivers Layer** (`kernel/hal/drivers/`):
    *   [console.c](file:///Users/olmo/Documents/git/ostest1/os1test-dev/kernel/hal/drivers/console.c): Generic screen print router.
    *   [uart/pl011.c](file:///Users/olmo/Documents/git/ostest1/os1test-dev/kernel/hal/drivers/uart/pl011.c): AArch64 serial UART driver.
    *   [uart/16550.c](file:///Users/olmo/Documents/git/ostest1/os1test-dev/kernel/hal/drivers/uart/16550.c): AMD64 serial COM driver.
    *   [gic/gic.c](file:///Users/olmo/Documents/git/ostest1/os1test-dev/kernel/hal/drivers/gic/gic.c): AArch64 Generic Interrupt Controller driver.
    *   [timer/pic_pit.c](file:///Users/olmo/Documents/git/ostest1/os1test-dev/kernel/hal/drivers/timer/pic_pit.c): AMD64 8253 PIT timer.
    *   [virtio/virtio_blk.c](file:///Users/olmo/Documents/git/ostest1/os1test-dev/kernel/hal/drivers/virtio/virtio_blk.c): VirtIO-Block device driver.
    *   [virtio/virtio_input.c](file:///Users/olmo/Documents/git/ostest1/os1test-dev/kernel/hal/drivers/virtio/virtio_input.c): VirtIO Keyboard/Mouse events wrapper.
    *   [gpu/virtio_gpu.c](file:///Users/olmo/Documents/git/ostest1/os1test-dev/kernel/hal/drivers/gpu/virtio_gpu.c): VirtIO-GPU 2D/3D framebuffer management.
*   **Relocated User Startup** (`kernel/hal/user/`):
    *   [init_asm.S](file:///Users/olmo/Documents/git/ostest1/os1test-dev/kernel/hal/user/init_asm.S): Userland process start entry frame.
    *   [arch/aarch64/syscall.S](file:///Users/olmo/Documents/git/ostest1/os1test-dev/kernel/hal/user/arch/aarch64/syscall.S): AArch64 assembly user syscall trampolines.
    *   [arch/amd64/syscall.S](file:///Users/olmo/Documents/git/ostest1/os1test-dev/kernel/hal/user/arch/amd64/syscall.S): AMD64 assembly user syscall trampolines.

### 3. Shared Library Primitives (`kernel/libkernel/`)
*   [registry.c](file:///Users/olmo/Documents/git/ostest1/os1test-dev/kernel/libkernel/src/registry.c): Plan 9 tree registry keys management (`RegKey` / `RegIpcQueue`).
*   [kmalloc.c](file:///Users/olmo/Documents/git/ostest1/os1test-dev/kernel/libkernel/src/kmalloc.c): Slab heap memory allocator.
*   [printk.c](file:///Users/olmo/Documents/git/ostest1/os1test-dev/kernel/libkernel/src/printk.c): Thread-safe log router.
*   [string.c](file:///Users/olmo/Documents/git/ostest1/os1test-dev/kernel/libkernel/src/string.c): Standard architecture-independent string manipulators.
*   [math.c](file:///Users/olmo/Documents/git/ostest1/os1test-dev/kernel/libkernel/src/math.c): Fixed-point geometry and trig.

### 4. User Space (`user/`)
*   **System Daemons** (`user/sys/bin/`):
    *   [init.c](file:///Users/olmo/Documents/git/ostest1/os1test-dev/user/sys/bin/init.c): PID 2 init orchestration manager.
    *   [notification_server.c](file:///Users/olmo/Documents/git/ostest1/os1test-dev/user/sys/bin/notification_server.c): Graphic notifications popup process.
    *   [shell.c](file:///Users/olmo/Documents/git/ostest1/os1test-dev/user/sys/bin/shell.c): Interactive terminal environment shell.
    *   [regedit.c](file:///Users/olmo/Documents/git/ostest1/user/sys/bin/regedit.c): Registry editor viewer utility.
*   **Standard Library Core** (`user/sys/lib/`):
    *   [malloc.c](file:///Users/olmo/Documents/git/ostest1/os1test-dev/user/sys/lib/malloc.c): Userland allocator wrapper.
    *   [lib.c](file:///Users/olmo/Documents/git/ostest1/os1test-dev/user/sys/lib/lib.c): Drawing, GUI windows and string facilities.
