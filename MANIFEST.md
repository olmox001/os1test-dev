.
├── ./MANIFEST.md
├── ./Makefile
├── ./README.md
├── ./REFACTOR_PLAN.md
├── ./boot
│   ├── ./boot/aarch64
│   │   ├── ./boot/aarch64/header.S
│   │   ├── ./boot/aarch64/linker.ld
│   │   ├── ./boot/aarch64/stage1.S
│   │   └── ./boot/aarch64/stage2.S
│   └── ./boot/amd64
│       ├── ./boot/amd64/boot.S
│       ├── ./boot/amd64/header.S
│       ├── ./boot/amd64/linker.ld
│       ├── ./boot/amd64/stage1.S
│       └── ./boot/amd64/stage2.S
├── ./build
│   ├── ./build/amd64
│   │   ├── ./build/amd64/boot
│   │   │   └── ./build/amd64/boot/amd64
│   │   │       ├── ./build/amd64/boot/amd64/header.o
│   │   │       ├── ./build/amd64/boot/amd64/stage1.o
│   │   │       └── ./build/amd64/boot/amd64/stage2.o
│   │   ├── ./build/amd64/bootloader.bin
│   │   ├── ./build/amd64/bootloader.elf
│   │   ├── ./build/amd64/kernel
│   │   │   ├── ./build/amd64/kernel/arch
│   │   │   │   └── ./build/amd64/kernel/arch/amd64
│   │   │   │       ├── ./build/amd64/kernel/arch/amd64/boot
│   │   │   │       │   ├── ./build/amd64/kernel/arch/amd64/boot/start.o
│   │   │   │       │   └── ./build/amd64/kernel/arch/amd64/boot/trampoline.o
│   │   │   │       ├── ./build/amd64/kernel/arch/amd64/cpu
│   │   │   │       │   ├── ./build/amd64/kernel/arch/amd64/cpu/apic.d
│   │   │   │       │   ├── ./build/amd64/kernel/arch/amd64/cpu/apic.o
│   │   │   │       │   ├── ./build/amd64/kernel/arch/amd64/cpu/context.o
│   │   │   │       │   ├── ./build/amd64/kernel/arch/amd64/cpu/cpu.d
│   │   │   │       │   ├── ./build/amd64/kernel/arch/amd64/cpu/cpu.o
│   │   │   │       │   ├── ./build/amd64/kernel/arch/amd64/cpu/gdt.d
│   │   │   │       │   ├── ./build/amd64/kernel/arch/amd64/cpu/gdt.o
│   │   │   │       │   ├── ./build/amd64/kernel/arch/amd64/cpu/idt.d
│   │   │   │       │   ├── ./build/amd64/kernel/arch/amd64/cpu/idt.o
│   │   │   │       │   ├── ./build/amd64/kernel/arch/amd64/cpu/isr_stubs.o
│   │   │   │       │   ├── ./build/amd64/kernel/arch/amd64/cpu/msr.d
│   │   │   │       │   ├── ./build/amd64/kernel/arch/amd64/cpu/msr.o
│   │   │   │       │   ├── ./build/amd64/kernel/arch/amd64/cpu/syscall.o
│   │   │   │       │   ├── ./build/amd64/kernel/arch/amd64/cpu/syscall_hal.d
│   │   │   │       │   └── ./build/amd64/kernel/arch/amd64/cpu/syscall_hal.o
│   │   │   │       ├── ./build/amd64/kernel/arch/amd64/drivers
│   │   │   │       ├── ./build/amd64/kernel/arch/amd64/hal.d
│   │   │   │       ├── ./build/amd64/kernel/arch/amd64/hal.o
│   │   │   │       ├── ./build/amd64/kernel/arch/amd64/mm
│   │   │   │       │   ├── ./build/amd64/kernel/arch/amd64/mm/mmu.d
│   │   │   │       │   ├── ./build/amd64/kernel/arch/amd64/mm/mmu.o
│   │   │   │       │   ├── ./build/amd64/kernel/arch/amd64/mm/uaccess.d
│   │   │   │       │   └── ./build/amd64/kernel/arch/amd64/mm/uaccess.o
│   │   │   │       ├── ./build/amd64/kernel/arch/amd64/platform
│   │   │   │       │   ├── ./build/amd64/kernel/arch/amd64/platform/platform.d
│   │   │   │       │   └── ./build/amd64/kernel/arch/amd64/platform/platform.o
│   │   │   │       ├── ./build/amd64/kernel/arch/amd64/virtio.d
│   │   │   │       └── ./build/amd64/kernel/arch/amd64/virtio.o
│   │   │   ├── ./build/amd64/kernel/core
│   │   │   │   ├── ./build/amd64/kernel/core/hal_bus.d
│   │   │   │   ├── ./build/amd64/kernel/core/hal_bus.o
│   │   │   │   ├── ./build/amd64/kernel/core/stubs.d
│   │   │   │   ├── ./build/amd64/kernel/core/stubs.o
│   │   │   │   ├── ./build/amd64/kernel/core/syscall.d
│   │   │   │   ├── ./build/amd64/kernel/core/syscall.o
│   │   │   │   ├── ./build/amd64/kernel/core/timer.d
│   │   │   │   └── ./build/amd64/kernel/core/timer.o
│   │   │   ├── ./build/amd64/kernel/cpu.d
│   │   │   ├── ./build/amd64/kernel/cpu.o
│   │   │   ├── ./build/amd64/kernel/drivers
│   │   │   │   ├── ./build/amd64/kernel/drivers/console.d
│   │   │   │   ├── ./build/amd64/kernel/drivers/console.o
│   │   │   │   ├── ./build/amd64/kernel/drivers/gic
│   │   │   │   ├── ./build/amd64/kernel/drivers/gpu
│   │   │   │   │   ├── ./build/amd64/kernel/drivers/gpu/gpu_core.d
│   │   │   │   │   ├── ./build/amd64/kernel/drivers/gpu/gpu_core.o
│   │   │   │   │   ├── ./build/amd64/kernel/drivers/gpu/virtio_gpu.d
│   │   │   │   │   └── ./build/amd64/kernel/drivers/gpu/virtio_gpu.o
│   │   │   │   ├── ./build/amd64/kernel/drivers/irq_ctrl.d
│   │   │   │   ├── ./build/amd64/kernel/drivers/irq_ctrl.o
│   │   │   │   ├── ./build/amd64/kernel/drivers/keyboard
│   │   │   │   │   ├── ./build/amd64/kernel/drivers/keyboard/keyboard.d
│   │   │   │   │   └── ./build/amd64/kernel/drivers/keyboard/keyboard.o
│   │   │   │   ├── ./build/amd64/kernel/drivers/pci
│   │   │   │   │   ├── ./build/amd64/kernel/drivers/pci/pci.d
│   │   │   │   │   └── ./build/amd64/kernel/drivers/pci/pci.o
│   │   │   │   ├── ./build/amd64/kernel/drivers/sys_timer.d
│   │   │   │   ├── ./build/amd64/kernel/drivers/sys_timer.o
│   │   │   │   ├── ./build/amd64/kernel/drivers/timer
│   │   │   │   │   ├── ./build/amd64/kernel/drivers/timer/pic_pit.d
│   │   │   │   │   └── ./build/amd64/kernel/drivers/timer/pic_pit.o
│   │   │   │   ├── ./build/amd64/kernel/drivers/uart
│   │   │   │   │   ├── ./build/amd64/kernel/drivers/uart/16550.d
│   │   │   │   │   └── ./build/amd64/kernel/drivers/uart/16550.o
│   │   │   │   └── ./build/amd64/kernel/drivers/virtio
│   │   │   │       ├── ./build/amd64/kernel/drivers/virtio/virtio_blk.d
│   │   │   │       ├── ./build/amd64/kernel/drivers/virtio/virtio_blk.o
│   │   │   │       ├── ./build/amd64/kernel/drivers/virtio/virtio_input.d
│   │   │   │       └── ./build/amd64/kernel/drivers/virtio/virtio_input.o
│   │   │   ├── ./build/amd64/kernel/fs
│   │   │   ├── ./build/amd64/kernel/graphics
│   │   │   │   ├── ./build/amd64/kernel/graphics/font.d
│   │   │   │   ├── ./build/amd64/kernel/graphics/font.o
│   │   │   │   ├── ./build/amd64/kernel/graphics/gl.d
│   │   │   │   ├── ./build/amd64/kernel/graphics/gl.o
│   │   │   │   ├── ./build/amd64/kernel/graphics/graphics.d
│   │   │   │   ├── ./build/amd64/kernel/graphics/graphics.o
│   │   │   │   ├── ./build/amd64/kernel/graphics/region.d
│   │   │   │   └── ./build/amd64/kernel/graphics/region.o
│   │   │   ├── ./build/amd64/kernel/irq
│   │   │   │   ├── ./build/amd64/kernel/irq/irq.d
│   │   │   │   └── ./build/amd64/kernel/irq/irq.o
│   │   │   ├── ./build/amd64/kernel/lib
│   │   │   │   ├── ./build/amd64/kernel/lib/fdt.d
│   │   │   │   ├── ./build/amd64/kernel/lib/fdt.o
│   │   │   │   ├── ./build/amd64/kernel/lib/kmalloc.d
│   │   │   │   ├── ./build/amd64/kernel/lib/kmalloc.o
│   │   │   │   ├── ./build/amd64/kernel/lib/printk.d
│   │   │   │   ├── ./build/amd64/kernel/lib/printk.o
│   │   │   │   ├── ./build/amd64/kernel/lib/registry.d
│   │   │   │   ├── ./build/amd64/kernel/lib/registry.o
│   │   │   │   ├── ./build/amd64/kernel/lib/stack_protector.d
│   │   │   │   ├── ./build/amd64/kernel/lib/stack_protector.o
│   │   │   │   ├── ./build/amd64/kernel/lib/vsnprintf.d
│   │   │   │   └── ./build/amd64/kernel/lib/vsnprintf.o
│   │   │   ├── ./build/amd64/kernel/libkernel
│   │   │   │   └── ./build/amd64/kernel/libkernel/src
│   │   │   │       ├── ./build/amd64/kernel/libkernel/src/crc32.d
│   │   │   │       ├── ./build/amd64/kernel/libkernel/src/crc32.o
│   │   │   │       ├── ./build/amd64/kernel/libkernel/src/math.d
│   │   │   │       ├── ./build/amd64/kernel/libkernel/src/math.o
│   │   │   │       ├── ./build/amd64/kernel/libkernel/src/string.d
│   │   │   │       ├── ./build/amd64/kernel/libkernel/src/string.o
│   │   │   │       ├── ./build/amd64/kernel/libkernel/src/utf8.d
│   │   │   │       └── ./build/amd64/kernel/libkernel/src/utf8.o
│   │   │   ├── ./build/amd64/kernel/main.d
│   │   │   ├── ./build/amd64/kernel/main.o
│   │   │   ├── ./build/amd64/kernel/mm
│   │   │   │   ├── ./build/amd64/kernel/mm/buffer.d
│   │   │   │   ├── ./build/amd64/kernel/mm/buffer.o
│   │   │   │   ├── ./build/amd64/kernel/mm/pmm.d
│   │   │   │   ├── ./build/amd64/kernel/mm/pmm.o
│   │   │   │   ├── ./build/amd64/kernel/mm/vmm.d
│   │   │   │   └── ./build/amd64/kernel/mm/vmm.o
│   │   │   └── ./build/amd64/kernel/sched
│   │   │       ├── ./build/amd64/kernel/sched/elf.d
│   │   │       ├── ./build/amd64/kernel/sched/elf.o
│   │   │       ├── ./build/amd64/kernel/sched/process.d
│   │   │       └── ./build/amd64/kernel/sched/process.o
│   │   ├── ./build/amd64/kernel.bin
│   │   ├── ./build/amd64/kernel.elf
│   │   └── ./build/amd64/user
│   │       ├── ./build/amd64/user/arch
│   │       │   └── ./build/amd64/user/arch/amd64
│   │       ├── ./build/amd64/user/bin
│   │       ├── ./build/amd64/user/lib
│   │       └── ./build/amd64/user/sys
│   │           ├── ./build/amd64/user/sys/bin
│   │           └── ./build/amd64/user/sys/lib
│   └── ./build/kernel.map
├── ./chek.sh
├── ./docs
│   ├── ./docs/report
│   │   ├── ./docs/report/SMP_AMD64_STABILIZATION_REPORT.md
│   │   └── ./docs/report/final_report_boot_stabilization.md
│   └── ./docs/screen
│       ├── ./docs/screen/Screenshot 2026-01-05 alle 08.30.41.png
│       ├── ./docs/screen/Screenshot 2026-01-05 alle 08.31.27.png
│       └── ./docs/screen/Screenshot 2026-05-10 alle 15.27.36.png
├── ./grub.cfg
├── ./include
│   └── ./include/api
│       ├── ./include/api/assert.h
│       ├── ./include/api/ctype.h
│       ├── ./include/api/elf.h
│       ├── ./include/api/errno.h
│       ├── ./include/api/fcntl.h
│       ├── ./include/api/font.h
│       ├── ./include/api/font_lib.h
│       ├── ./include/api/graphics.h
│       ├── ./include/api/input.h
│       ├── ./include/api/inttypes.h
│       ├── ./include/api/math.h
│       ├── ./include/api/os1.h
│       ├── ./include/api/posix_types.h
│       ├── ./include/api/posix_types.h.old
│       ├── ./include/api/stb_easy_font.h
│       ├── ./include/api/stb_image.h
│       ├── ./include/api/stdbool.h
│       ├── ./include/api/stdio.h
│       ├── ./include/api/stdlib.h
│       ├── ./include/api/string.h
│       ├── ./include/api/strings.h
│       ├── ./include/api/sys
│       │   ├── ./include/api/sys/stat.h
│       │   └── ./include/api/sys/types.h
│       └── ./include/api/unistd.h
├── ./kernel
│   ├── ./kernel/arch
│   │   ├── ./kernel/arch/aarch64
│   │   │   ├── ./kernel/arch/aarch64/boot
│   │   │   │   └── ./kernel/arch/aarch64/boot/start.S
│   │   │   ├── ./kernel/arch/aarch64/cpu
│   │   │   │   ├── ./kernel/arch/aarch64/cpu/cpu.c
│   │   │   │   ├── ./kernel/arch/aarch64/cpu/exception.S
│   │   │   │   └── ./kernel/arch/aarch64/cpu/syscall.c
│   │   │   ├── ./kernel/arch/aarch64/hal.c
│   │   │   ├── ./kernel/arch/aarch64/include
│   │   │   │   └── ./kernel/arch/aarch64/include/arch
│   │   │   │       ├── ./kernel/arch/aarch64/include/arch/arch.h
│   │   │   │       ├── ./kernel/arch/aarch64/include/arch/platform.h
│   │   │   │       └── ./kernel/arch/aarch64/include/arch/pt_regs.h
│   │   │   ├── ./kernel/arch/aarch64/kernel.ld
│   │   │   ├── ./kernel/arch/aarch64/mm
│   │   │   │   └── ./kernel/arch/aarch64/mm/mmu.c
│   │   │   ├── ./kernel/arch/aarch64/platform.c
│   │   │   └── ./kernel/arch/aarch64/virtio.c
│   │   └── ./kernel/arch/amd64
│   │       ├── ./kernel/arch/amd64/boot
│   │       │   ├── ./kernel/arch/amd64/boot/start.S
│   │       │   └── ./kernel/arch/amd64/boot/trampoline.S
│   │       ├── ./kernel/arch/amd64/cpu
│   │       │   ├── ./kernel/arch/amd64/cpu/apic.c
│   │       │   ├── ./kernel/arch/amd64/cpu/context.S
│   │       │   ├── ./kernel/arch/amd64/cpu/cpu.c
│   │       │   ├── ./kernel/arch/amd64/cpu/gdt.c
│   │       │   ├── ./kernel/arch/amd64/cpu/gdt_defs.h
│   │       │   ├── ./kernel/arch/amd64/cpu/idt.c
│   │       │   ├── ./kernel/arch/amd64/cpu/isr_stubs.S
│   │       │   ├── ./kernel/arch/amd64/cpu/msr.c
│   │       │   ├── ./kernel/arch/amd64/cpu/syscall.S
│   │       │   └── ./kernel/arch/amd64/cpu/syscall_hal.c
│   │       ├── ./kernel/arch/amd64/hal.c
│   │       ├── ./kernel/arch/amd64/include
│   │       │   └── ./kernel/arch/amd64/include/arch
│   │       │       ├── ./kernel/arch/amd64/include/arch/amd64_internal.h
│   │       │       ├── ./kernel/arch/amd64/include/arch/arch.h
│   │       │       ├── ./kernel/arch/amd64/include/arch/platform.h
│   │       │       └── ./kernel/arch/amd64/include/arch/pt_regs.h
│   │       ├── ./kernel/arch/amd64/kernel.ld
│   │       ├── ./kernel/arch/amd64/mm
│   │       │   ├── ./kernel/arch/amd64/mm/mmu.c
│   │       │   └── ./kernel/arch/amd64/mm/uaccess.c
│   │       ├── ./kernel/arch/amd64/platform
│   │       │   └── ./kernel/arch/amd64/platform/platform.c
│   │       └── ./kernel/arch/amd64/virtio.c
│   ├── ./kernel/core
│   │   ├── ./kernel/core/hal_bus.c
│   │   ├── ./kernel/core/stubs.c
│   │   ├── ./kernel/core/syscall.c
│   │   ├── ./kernel/core/syscall_dispatch.c.old
│   │   └── ./kernel/core/timer.c
│   ├── ./kernel/cpu.c
│   ├── ./kernel/drivers
│   │   ├── ./kernel/drivers/console.c
│   │   ├── ./kernel/drivers/cpp_test.cpp
│   │   ├── ./kernel/drivers/gic
│   │   │   ├── ./kernel/drivers/gic/gic.c
│   │   │   └── ./kernel/drivers/gic/gic_regs.h
│   │   ├── ./kernel/drivers/gpu
│   │   │   ├── ./kernel/drivers/gpu/gpu_core.c
│   │   │   └── ./kernel/drivers/gpu/virtio_gpu.c
│   │   ├── ./kernel/drivers/irq_ctrl.c
│   │   ├── ./kernel/drivers/keyboard
│   │   │   └── ./kernel/drivers/keyboard/keyboard.c
│   │   ├── ./kernel/drivers/pci
│   │   │   └── ./kernel/drivers/pci/pci.c
│   │   ├── ./kernel/drivers/sys_timer.c
│   │   ├── ./kernel/drivers/timer
│   │   │   ├── ./kernel/drivers/timer/pic_pit.c
│   │   │   └── ./kernel/drivers/timer/timer.c
│   │   ├── ./kernel/drivers/uart
│   │   │   ├── ./kernel/drivers/uart/16550.c
│   │   │   └── ./kernel/drivers/uart/pl011.c
│   │   └── ./kernel/drivers/virtio
│   │       ├── ./kernel/drivers/virtio/virtio_blk.c
│   │       └── ./kernel/drivers/virtio/virtio_input.c
│   ├── ./kernel/fs
│   │   ├── ./kernel/fs/ext4.c.old
│   │   ├── ./kernel/fs/gpt.c
│   │   └── ./kernel/fs/vfs.c.old
│   ├── ./kernel/graphics
│   │   ├── ./kernel/graphics/compositor.c.old
│   │   ├── ./kernel/graphics/draw2d.c
│   │   ├── ./kernel/graphics/draw3d.c
│   │   ├── ./kernel/graphics/font.c
│   │   ├── ./kernel/graphics/gl.c
│   │   ├── ./kernel/graphics/graphics.c
│   │   └── ./kernel/graphics/region.c
│   ├── ./kernel/include
│   │   ├── ./kernel/include/arch
│   │   │   └── ./kernel/include/arch/amd64
│   │   │       └── ./kernel/include/arch/amd64/apic.h
│   │   ├── ./kernel/include/drivers
│   │   │   ├── ./kernel/include/drivers/gic.h
│   │   │   ├── ./kernel/include/drivers/gpu
│   │   │   │   └── ./kernel/include/drivers/gpu/gpu.h
│   │   │   ├── ./kernel/include/drivers/keyboard.h
│   │   │   ├── ./kernel/include/drivers/pci.h
│   │   │   ├── ./kernel/include/drivers/timer.h
│   │   │   ├── ./kernel/include/drivers/uart.h
│   │   │   ├── ./kernel/include/drivers/virtio.h
│   │   │   ├── ./kernel/include/drivers/virtio_blk.h
│   │   │   ├── ./kernel/include/drivers/virtio_gpu.h
│   │   │   ├── ./kernel/include/drivers/virtio_hal.h
│   │   │   └── ./kernel/include/drivers/virtio_input.h
│   │   ├── ./kernel/include/graphics
│   │   │   ├── ./kernel/include/graphics/default_font.h
│   │   │   └── ./kernel/include/graphics/gl.h
│   │   └── ./kernel/include/kernel
│   │       ├── ./kernel/include/kernel/arch.h
│   │       ├── ./kernel/include/kernel/buffer.h
│   │       ├── ./kernel/include/kernel/cpu.h
│   │       ├── ./kernel/include/kernel/drivers.h
│   │       ├── ./kernel/include/kernel/elf.h
│   │       ├── ./kernel/include/kernel/errno.h
│   │       ├── ./kernel/include/kernel/ext4.h
│   │       ├── ./kernel/include/kernel/fdt.h
│   │       ├── ./kernel/include/kernel/gpt.h
│   │       ├── ./kernel/include/kernel/graphics.h
│   │       ├── ./kernel/include/kernel/hal.h
│   │       ├── ./kernel/include/kernel/hal_device.h
│   │       ├── ./kernel/include/kernel/hal_platform.h
│   │       ├── ./kernel/include/kernel/hal_unified.h
│   │       ├── ./kernel/include/kernel/ipc.h
│   │       ├── ./kernel/include/kernel/irq.h
│   │       ├── ./kernel/include/kernel/kmalloc.h
│   │       ├── ./kernel/include/kernel/list.h
│   │       ├── ./kernel/include/kernel/math.h
│   │       ├── ./kernel/include/kernel/multiboot2.h
│   │       ├── ./kernel/include/kernel/platform.h
│   │       ├── ./kernel/include/kernel/pmm.h
│   │       ├── ./kernel/include/kernel/printk.h
│   │       ├── ./kernel/include/kernel/region.h
│   │       ├── ./kernel/include/kernel/registry.h
│   │       ├── ./kernel/include/kernel/sched.h
│   │       ├── ./kernel/include/kernel/spinlock.h
│   │       ├── ./kernel/include/kernel/string.h
│   │       ├── ./kernel/include/kernel/test.h
│   │       ├── ./kernel/include/kernel/timer.h
│   │       ├── ./kernel/include/kernel/types.h
│   │       ├── ./kernel/include/kernel/types.h.old
│   │       ├── ./kernel/include/kernel/vfs.h
│   │       ├── ./kernel/include/kernel/virtio_hal.h
│   │       └── ./kernel/include/kernel/vmm.h
│   ├── ./kernel/irq
│   │   └── ./kernel/irq/irq.c
│   ├── ./kernel/kernel.o
│   ├── ./kernel/lib
│   │   ├── ./kernel/lib/crc32.c.old
│   │   ├── ./kernel/lib/fdt.c
│   │   ├── ./kernel/lib/kmalloc.c
│   │   ├── ./kernel/lib/ktest.c
│   │   ├── ./kernel/lib/ktest_samples.c
│   │   ├── ./kernel/lib/math.c.old
│   │   ├── ./kernel/lib/printk.c
│   │   ├── ./kernel/lib/registry.c
│   │   ├── ./kernel/lib/stack_protector.c
│   │   ├── ./kernel/lib/string.c.old
│   │   ├── ./kernel/lib/utf8.c.old
│   │   └── ./kernel/lib/vsnprintf.c
│   ├── ./kernel/libkernel
│   │   ├── ./kernel/libkernel/include
│   │   │   ├── ./kernel/libkernel/include/math.h
│   │   │   ├── ./kernel/libkernel/include/string.h
│   │   │   └── ./kernel/libkernel/include/types.h
│   │   └── ./kernel/libkernel/src
│   │       ├── ./kernel/libkernel/src/crc32.c
│   │       ├── ./kernel/libkernel/src/math.c
│   │       ├── ./kernel/libkernel/src/string.c
│   │       └── ./kernel/libkernel/src/utf8.c
│   ├── ./kernel/main.c
│   ├── ./kernel/main.c.old
│   ├── ./kernel/mm
│   │   ├── ./kernel/mm/buffer.c
│   │   ├── ./kernel/mm/pmm.c
│   │   └── ./kernel/mm/vmm.c
│   └── ./kernel/sched
│       ├── ./kernel/sched/elf.c
│       └── ./kernel/sched/process.c
├── ./logs
│   ├── ./logs/test_aarch64.txt
│   ├── ./logs/test_aarch64_phaseA.txt
│   ├── ./logs/test_aarch64_v10.txt
│   ├── ./logs/test_aarch64_v11.txt
│   ├── ./logs/test_aarch64_v12.txt
│   ├── ./logs/test_aarch64_v13.txt
│   ├── ./logs/test_aarch64_v2.txt
│   ├── ./logs/test_aarch64_v3.txt
│   ├── ./logs/test_aarch64_v4.txt
│   ├── ./logs/test_aarch64_v5.txt
│   ├── ./logs/test_aarch64_v6.txt
│   ├── ./logs/test_aarch64_v7.txt
│   ├── ./logs/test_aarch64_v8.txt
│   ├── ./logs/test_aarch64_v9.txt
│   ├── ./logs/test_amd64.txt
│   ├── ./logs/test_amd64_phaseA.txt
│   ├── ./logs/test_amd64_v2.txt
│   ├── ./logs/test_amd64_vfinal.txt
│   ├── ./logs/test_amd64_vfinal_fixed.txt
│   └── ./logs/test_amd64_vfinal_fixed_v2.txt
├── ./scratch
│   ├── ./scratch/check_sizes
│   └── ./scratch/check_sizes.c
├── ./tools
│   ├── ./tools/kernel_doctor
│   │   ├── ./tools/kernel_doctor/install_deps.sh
│   │   └── ./tools/kernel_doctor/kernel_doctor.py
│   ├── ./tools/kernel_validator.py
│   ├── ./tools/make-bootable-img.sh
│   ├── ./tools/make-bootable-iso.sh
│   ├── ./tools/mkdisk.c
│   ├── ./tools/mkfont
│   ├── ./tools/mkfont.c
│   ├── ./tools/stb_truetype.h
│   ├── ./tools/ttf2off
│   └── ./tools/ttf2off.c
└── ./user
    ├── ./user/arch
    │   ├── ./user/arch/aarch64
    │   │   └── ./user/arch/aarch64/syscall.S
    │   └── ./user/arch/amd64
    │       └── ./user/arch/amd64/syscall.S
    ├── ./user/bin
    │   ├── ./user/bin/counter.c
    │   ├── ./user/bin/crash.c
    │   ├── ./user/bin/demo3d.c
    │   ├── ./user/bin/doom
    │   │   ├── ./user/bin/doom/am_map.c
    │   │   ├── ./user/bin/doom/am_map.h
    │   │   ├── ./user/bin/doom/chex.wad
    │   │   ├── ./user/bin/doom/chex3.wad
    │   │   ├── ./user/bin/doom/config.h
    │   │   ├── ./user/bin/doom/d_englsh.h
    │   │   ├── ./user/bin/doom/d_event.c
    │   │   ├── ./user/bin/doom/d_event.h
    │   │   ├── ./user/bin/doom/d_items.c
    │   │   ├── ./user/bin/doom/d_items.h
    │   │   ├── ./user/bin/doom/d_iwad.c
    │   │   ├── ./user/bin/doom/d_iwad.h
    │   │   ├── ./user/bin/doom/d_loop.c
    │   │   ├── ./user/bin/doom/d_loop.h
    │   │   ├── ./user/bin/doom/d_main.c
    │   │   ├── ./user/bin/doom/d_main.h
    │   │   ├── ./user/bin/doom/d_mode.c
    │   │   ├── ./user/bin/doom/d_mode.h
    │   │   ├── ./user/bin/doom/d_net.c
    │   │   ├── ./user/bin/doom/d_player.h
    │   │   ├── ./user/bin/doom/d_textur.h
    │   │   ├── ./user/bin/doom/d_think.h
    │   │   ├── ./user/bin/doom/d_ticcmd.h
    │   │   ├── ./user/bin/doom/deh_main.h
    │   │   ├── ./user/bin/doom/deh_misc.h
    │   │   ├── ./user/bin/doom/deh_str.h
    │   │   ├── ./user/bin/doom/doom-nexs-aarch64.make
    │   │   ├── ./user/bin/doom/doom-nexs-amd64.make
    │   │   ├── ./user/bin/doom/doom.h
    │   │   ├── ./user/bin/doom/doom.wad
    │   │   ├── ./user/bin/doom/doom1.wad
    │   │   ├── ./user/bin/doom/doom2.wad
    │   │   ├── ./user/bin/doom/doom2f.wad
    │   │   ├── ./user/bin/doom/doomdata.h
    │   │   ├── ./user/bin/doom/doomdef.c
    │   │   ├── ./user/bin/doom/doomdef.h
    │   │   ├── ./user/bin/doom/doomfeatures.h
    │   │   ├── ./user/bin/doom/doomgeneric.c
    │   │   ├── ./user/bin/doom/doomgeneric.h
    │   │   ├── ./user/bin/doom/doomgeneric_os1.c
    │   │   ├── ./user/bin/doom/doomkeys.h
    │   │   ├── ./user/bin/doom/doomstat.c
    │   │   ├── ./user/bin/doom/doomstat.h
    │   │   ├── ./user/bin/doom/doomtype.h
    │   │   ├── ./user/bin/doom/doomu.wad
    │   │   ├── ./user/bin/doom/dstrings.c
    │   │   ├── ./user/bin/doom/dstrings.h
    │   │   ├── ./user/bin/doom/dummy.c
    │   │   ├── ./user/bin/doom/f_finale.c
    │   │   ├── ./user/bin/doom/f_finale.h
    │   │   ├── ./user/bin/doom/f_wipe.c
    │   │   ├── ./user/bin/doom/f_wipe.h
    │   │   ├── ./user/bin/doom/g_game.c
    │   │   ├── ./user/bin/doom/g_game.h
    │   │   ├── ./user/bin/doom/gusconf.c
    │   │   ├── ./user/bin/doom/gusconf.h
    │   │   ├── ./user/bin/doom/heretic.wad
    │   │   ├── ./user/bin/doom/heretic1.wad
    │   │   ├── ./user/bin/doom/hexdd.wad
    │   │   ├── ./user/bin/doom/hexen.wad
    │   │   ├── ./user/bin/doom/hexendemo.wad
    │   │   ├── ./user/bin/doom/hu_lib.c
    │   │   ├── ./user/bin/doom/hu_lib.h
    │   │   ├── ./user/bin/doom/hu_stuff.c
    │   │   ├── ./user/bin/doom/hu_stuff.h
    │   │   ├── ./user/bin/doom/i_allegromusic.c
    │   │   ├── ./user/bin/doom/i_allegrosound.c
    │   │   ├── ./user/bin/doom/i_cdmus.c
    │   │   ├── ./user/bin/doom/i_cdmus.h
    │   │   ├── ./user/bin/doom/i_endoom.c
    │   │   ├── ./user/bin/doom/i_endoom.h
    │   │   ├── ./user/bin/doom/i_input.c
    │   │   ├── ./user/bin/doom/i_joystick.c
    │   │   ├── ./user/bin/doom/i_joystick.h
    │   │   ├── ./user/bin/doom/i_scale.c
    │   │   ├── ./user/bin/doom/i_scale.h
    │   │   ├── ./user/bin/doom/i_sdlmusic.c
    │   │   ├── ./user/bin/doom/i_sdlsound.c
    │   │   ├── ./user/bin/doom/i_sound.c
    │   │   ├── ./user/bin/doom/i_sound.h
    │   │   ├── ./user/bin/doom/i_swap.h
    │   │   ├── ./user/bin/doom/i_system.c
    │   │   ├── ./user/bin/doom/i_system.h
    │   │   ├── ./user/bin/doom/i_timer.c
    │   │   ├── ./user/bin/doom/i_timer.h
    │   │   ├── ./user/bin/doom/i_video.c
    │   │   ├── ./user/bin/doom/i_video.h
    │   │   ├── ./user/bin/doom/icon.c
    │   │   ├── ./user/bin/doom/info.c
    │   │   ├── ./user/bin/doom/info.h
    │   │   ├── ./user/bin/doom/m_argv.c
    │   │   ├── ./user/bin/doom/m_argv.h
    │   │   ├── ./user/bin/doom/m_bbox.c
    │   │   ├── ./user/bin/doom/m_bbox.h
    │   │   ├── ./user/bin/doom/m_cheat.c
    │   │   ├── ./user/bin/doom/m_cheat.h
    │   │   ├── ./user/bin/doom/m_config.c
    │   │   ├── ./user/bin/doom/m_config.h
    │   │   ├── ./user/bin/doom/m_controls.c
    │   │   ├── ./user/bin/doom/m_controls.h
    │   │   ├── ./user/bin/doom/m_fixed.c
    │   │   ├── ./user/bin/doom/m_fixed.h
    │   │   ├── ./user/bin/doom/m_menu.c
    │   │   ├── ./user/bin/doom/m_menu.h
    │   │   ├── ./user/bin/doom/m_misc.c
    │   │   ├── ./user/bin/doom/m_misc.h
    │   │   ├── ./user/bin/doom/m_random.c
    │   │   ├── ./user/bin/doom/m_random.h
    │   │   ├── ./user/bin/doom/memio.c
    │   │   ├── ./user/bin/doom/memio.h
    │   │   ├── ./user/bin/doom/mus2mid.c
    │   │   ├── ./user/bin/doom/mus2mid.h
    │   │   ├── ./user/bin/doom/net_client.h
    │   │   ├── ./user/bin/doom/net_dedicated.h
    │   │   ├── ./user/bin/doom/net_defs.h
    │   │   ├── ./user/bin/doom/net_gui.h
    │   │   ├── ./user/bin/doom/net_io.h
    │   │   ├── ./user/bin/doom/net_loop.h
    │   │   ├── ./user/bin/doom/net_packet.h
    │   │   ├── ./user/bin/doom/net_query.h
    │   │   ├── ./user/bin/doom/net_sdl.h
    │   │   ├── ./user/bin/doom/net_server.h
    │   │   ├── ./user/bin/doom/p_ceilng.c
    │   │   ├── ./user/bin/doom/p_doors.c
    │   │   ├── ./user/bin/doom/p_enemy.c
    │   │   ├── ./user/bin/doom/p_floor.c
    │   │   ├── ./user/bin/doom/p_inter.c
    │   │   ├── ./user/bin/doom/p_inter.h
    │   │   ├── ./user/bin/doom/p_lights.c
    │   │   ├── ./user/bin/doom/p_local.h
    │   │   ├── ./user/bin/doom/p_map.c
    │   │   ├── ./user/bin/doom/p_maputl.c
    │   │   ├── ./user/bin/doom/p_mobj.c
    │   │   ├── ./user/bin/doom/p_mobj.h
    │   │   ├── ./user/bin/doom/p_plats.c
    │   │   ├── ./user/bin/doom/p_pspr.c
    │   │   ├── ./user/bin/doom/p_pspr.h
    │   │   ├── ./user/bin/doom/p_saveg.c
    │   │   ├── ./user/bin/doom/p_saveg.h
    │   │   ├── ./user/bin/doom/p_setup.c
    │   │   ├── ./user/bin/doom/p_setup.h
    │   │   ├── ./user/bin/doom/p_sight.c
    │   │   ├── ./user/bin/doom/p_spec.c
    │   │   ├── ./user/bin/doom/p_spec.h
    │   │   ├── ./user/bin/doom/p_switch.c
    │   │   ├── ./user/bin/doom/p_telept.c
    │   │   ├── ./user/bin/doom/p_tick.c
    │   │   ├── ./user/bin/doom/p_tick.h
    │   │   ├── ./user/bin/doom/p_user.c
    │   │   ├── ./user/bin/doom/plutonia.wad
    │   │   ├── ./user/bin/doom/r_bsp.c
    │   │   ├── ./user/bin/doom/r_bsp.h
    │   │   ├── ./user/bin/doom/r_data.c
    │   │   ├── ./user/bin/doom/r_data.h
    │   │   ├── ./user/bin/doom/r_defs.h
    │   │   ├── ./user/bin/doom/r_draw.c
    │   │   ├── ./user/bin/doom/r_draw.h
    │   │   ├── ./user/bin/doom/r_local.h
    │   │   ├── ./user/bin/doom/r_main.c
    │   │   ├── ./user/bin/doom/r_main.h
    │   │   ├── ./user/bin/doom/r_plane.c
    │   │   ├── ./user/bin/doom/r_plane.h
    │   │   ├── ./user/bin/doom/r_segs.c
    │   │   ├── ./user/bin/doom/r_segs.h
    │   │   ├── ./user/bin/doom/r_sky.c
    │   │   ├── ./user/bin/doom/r_sky.h
    │   │   ├── ./user/bin/doom/r_state.h
    │   │   ├── ./user/bin/doom/r_things.c
    │   │   ├── ./user/bin/doom/r_things.h
    │   │   ├── ./user/bin/doom/s_sound.c
    │   │   ├── ./user/bin/doom/s_sound.h
    │   │   ├── ./user/bin/doom/sha1.c
    │   │   ├── ./user/bin/doom/sha1.h
    │   │   ├── ./user/bin/doom/sounds.c
    │   │   ├── ./user/bin/doom/sounds.h
    │   │   ├── ./user/bin/doom/st_lib.c
    │   │   ├── ./user/bin/doom/st_lib.h
    │   │   ├── ./user/bin/doom/st_stuff.c
    │   │   ├── ./user/bin/doom/st_stuff.h
    │   │   ├── ./user/bin/doom/statdump.c
    │   │   ├── ./user/bin/doom/statdump.h
    │   │   ├── ./user/bin/doom/strife0.wad
    │   │   ├── ./user/bin/doom/strife1.wad
    │   │   ├── ./user/bin/doom/tables.c
    │   │   ├── ./user/bin/doom/tables.h
    │   │   ├── ./user/bin/doom/tnt.wad
    │   │   ├── ./user/bin/doom/v_patch.h
    │   │   ├── ./user/bin/doom/v_video.c
    │   │   ├── ./user/bin/doom/v_video.h
    │   │   ├── ./user/bin/doom/voices.wad
    │   │   ├── ./user/bin/doom/w_checksum.c
    │   │   ├── ./user/bin/doom/w_checksum.h
    │   │   ├── ./user/bin/doom/w_file.c
    │   │   ├── ./user/bin/doom/w_file.h
    │   │   ├── ./user/bin/doom/w_file_stdc.c
    │   │   ├── ./user/bin/doom/w_main.c
    │   │   ├── ./user/bin/doom/w_main.h
    │   │   ├── ./user/bin/doom/w_merge.h
    │   │   ├── ./user/bin/doom/w_wad.c
    │   │   ├── ./user/bin/doom/w_wad.h
    │   │   ├── ./user/bin/doom/wi_stuff.c
    │   │   ├── ./user/bin/doom/wi_stuff.h
    │   │   ├── ./user/bin/doom/z_zone.c
    │   │   └── ./user/bin/doom/z_zone.h
    │   ├── ./user/bin/doomgeneric-master
    │   │   └── ./user/bin/doomgeneric-master/doomgeneric
    │   ├── ./user/bin/input_test.c
    │   ├── ./user/bin/ipc_recv.c
    │   ├── ./user/bin/ipc_send.c
    │   ├── ./user/bin/test_init.c
    │   └── ./user/bin/writetest.c
    ├── ./user/hal
    ├── ./user/include
    ├── ./user/init_asm.S
    └── ./user/sys
        ├── ./user/sys/bin
        │   ├── ./user/sys/bin/fontman
        │   │   ├── ./user/sys/bin/fontman/fontman.c
        │   │   └── ./user/sys/bin/fontman/fonts
        │   │       ├── ./user/sys/bin/fontman/fonts/Orkney Bold Italic.ttf
        │   │       ├── ./user/sys/bin/fontman/fonts/Orkney Bold.ttf
        │   │       ├── ./user/sys/bin/fontman/fonts/Orkney Light Italic.ttf
        │   │       ├── ./user/sys/bin/fontman/fonts/Orkney Light.ttf
        │   │       ├── ./user/sys/bin/fontman/fonts/Orkney Medium Italic.ttf
        │   │       ├── ./user/sys/bin/fontman/fonts/Orkney Medium.ttf
        │   │       ├── ./user/sys/bin/fontman/fonts/Orkney Regular Italic.ttf
        │   │       ├── ./user/sys/bin/fontman/fonts/Rewir-Light.off
        │   │       ├── ./user/sys/bin/fontman/fonts/Rewir-Light.ttf
        │   │       └── ./user/sys/bin/fontman/fonts/basic.ttf
        │   ├── ./user/sys/bin/init.c
        │   ├── ./user/sys/bin/init.cfg
        │   ├── ./user/sys/bin/notification_server.c
        │   ├── ./user/sys/bin/proce.c
        │   ├── ./user/sys/bin/proce.h
        │   ├── ./user/sys/bin/regedit.c
        │   └── ./user/sys/bin/shell.c
        ├── ./user/sys/include
        │   ├── ./user/sys/include/abi.h
        │   ├── ./user/sys/include/errno.h
        │   └── ./user/sys/include/ipc.h
        └── ./user/sys/lib
            ├── ./user/sys/lib/font_lib.c
            ├── ./user/sys/lib/lib.c
            ├── ./user/sys/lib/lib.h
            ├── ./user/sys/lib/malloc.c
            └── ./user/sys/lib/syscall.S

107 directories, 570 files
