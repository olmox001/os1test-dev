# Makefile for OS1Test (AMD64 / AArch64)
# Cross-compilation for bare-metal kernel
# ==============================================================================
# Configuration
# ==============================================================================

# Target Architecture (default to aarch64)
ARCH ?= aarch64

# Release Versioning (default V9.9.9)
VERSION ?= V9.9.9
RELEASE_BASE = release/$(VERSION)
RELEASE_DIR = $(RELEASE_BASE)/$(ARCH)

# Normalize ARCH typos
ifeq ($(findstring aarch64,$(ARCH)),aarch64)
    override ARCH := aarch64
else ifeq ($(findstring amd64,$(ARCH)),amd64)
    override ARCH := amd64
endif

# Common Compiler Flags
COMMON_FLAGS = -Wall -Wextra -Werror -Wshadow -Wwrite-strings \
               -Wmissing-prototypes -Wstrict-prototypes -std=gnu11 \
               -ffreestanding -fno-builtin -nostdlib -nostartfiles \
               -fno-common -fstack-protector-strong \
               -fno-pic -fno-pie \
               -fno-omit-frame-pointer \
               -O2 -g

ifeq ($(ARCH), amd64)
    CROSS_COMPILE ?= x86_64-elf-
    KERNEL_DIR = kernel
    BOOT_DIR   = $(KERNEL_DIR)/hal/boot/amd64
    ARCH_DIR   = $(KERNEL_DIR)/hal/arch/amd64
    
    ARCH_CFLAGS = -DARCH_AMD64 -mno-red-zone -mcmodel=large
    ASFLAGS = -g --fatal-warnings
    
    LDFLAGS_BOOT = -nostdlib -static -z noexecstack -T $(BOOT_DIR)/linker.ld
    LDFLAGS_KERN = -nostdlib -static -z noexecstack -T $(ARCH_DIR)/kernel.ld -Map build/kernel.map
    
    CFLAGS_BOOT = $(filter-out -mcmodel=large,$(COMMON_FLAGS)) $(INCLUDE) -m32
    QEMU = qemu-system-x86_64
else
    CROSS_COMPILE ?= aarch64-none-elf-
    KERNEL_DIR = kernel
    BOOT_DIR   = $(KERNEL_DIR)/hal/boot/aarch64
    ARCH_DIR   = $(KERNEL_DIR)/hal/arch/aarch64
    
    ARCH_CFLAGS = -DARCH_AARCH64 -mcpu=cortex-a57
    ASFLAGS = -mcpu=cortex-a57 -g --fatal-warnings
    
    LDFLAGS_BOOT = -nostdlib -static -z noexecstack -T $(BOOT_DIR)/linker.ld
    LDFLAGS_KERN = -nostdlib -static -z noexecstack -T $(ARCH_DIR)/kernel.ld -Map build/kernel.map
    
    CFLAGS_BOOT = $(COMMON_FLAGS) $(INCLUDE)
    QEMU = qemu-system-aarch64
endif

CFLAGS = $(COMMON_FLAGS) $(ARCH_CFLAGS) $(KERNEL_INCLUDE)
CXXFLAGS = $(COMMON_FLAGS) $(ARCH_CFLAGS) $(KERNEL_INCLUDE) -fno-exceptions -fno-rtti
USER_CFLAGS = $(COMMON_FLAGS) $(ARCH_CFLAGS) $(USER_INCLUDE)

# Tools
CC      = $(CROSS_COMPILE)gcc
CXX     = $(CROSS_COMPILE)g++
AS      = $(CROSS_COMPILE)as
LD      = $(CROSS_COMPILE)ld
OBJCOPY = $(CROSS_COMPILE)objcopy
OBJDUMP = $(CROSS_COMPILE)objdump
MKDISK  = tools/mkdisk

# Resolve grub-mkrescue
GRUB_MKRESCUE := $(shell command -v i686-elf-grub-mkrescue 2>/dev/null || command -v grub-mkrescue 2>/dev/null || echo "")

# Directories
BUILD_ROOT = build
BUILD_DIR  = $(BUILD_ROOT)/$(ARCH)
USER_DIR   = user
USER_SYS_DIR = $(USER_DIR)/sys
USER_LIB_DIR = $(USER_SYS_DIR)/lib
USER_BIN_DIR = $(USER_SYS_DIR)/bin
USER_ARCH_DIR = $(KERNEL_DIR)/hal/user/arch/$(ARCH)
# Kernel-only include paths — NEVER includes user/sys/include (namespace isolation)
KERNEL_INCLUDE = -Ikernel/core/include -Ikernel/hal/include -Ikernel/hal/arch/$(ARCH)/include -Ikernel/libkernel/include -Ikernel/module/include
# Userland include paths — includes both kernel shared headers and user headers
USER_INCLUDE   = $(KERNEL_INCLUDE) -Iuser/sys/include
# INCLUDE defaults to kernel-only for backward compat with boot rules
INCLUDE    = $(KERNEL_INCLUDE)

# Output files
BOOTLOADER_ELF = $(BUILD_DIR)/bootloader.elf
BOOTLOADER_BIN = $(BUILD_DIR)/bootloader.bin
KERNEL_ELF = $(BUILD_DIR)/kernel.elf
KERNEL_BIN = $(BUILD_DIR)/kernel.bin
USER_ELF   = $(BUILD_DIR)/init.elf
DISK_IMG   = $(BUILD_DIR)/disk.img

# ==============================================================================
# Source Files
# ==============================================================================

# --- Boot & Assembly Sources ---
ifeq ($(ARCH), amd64)
BOOT_SOURCES = \
    $(BOOT_DIR)/header.S \
    $(BOOT_DIR)/stage1.S \
    $(BOOT_DIR)/stage2.S

KERN_ASM_SOURCES = \
    $(ARCH_DIR)/boot/start.S \
    $(ARCH_DIR)/cpu/isr_stubs.S \
    $(ARCH_DIR)/cpu/syscall.S \
    $(ARCH_DIR)/cpu/context.S \
    $(ARCH_DIR)/boot/trampoline.S
else
BOOT_SOURCES = \
    $(BOOT_DIR)/header.S \
    $(BOOT_DIR)/stage1.S \
    $(BOOT_DIR)/stage2.S

KERN_ASM_SOURCES = \
    $(ARCH_DIR)/boot/start.S \
    $(ARCH_DIR)/cpu/exception.S
endif

# --- HAL Sources (Architecture-Specific only — no drivers, no mm/irq) ---
ifeq ($(ARCH), amd64)
HAL_SOURCES = \
    $(ARCH_DIR)/cpu/cpu.c \
    $(ARCH_DIR)/cpu/idt.c \
    $(ARCH_DIR)/cpu/gdt.c \
    $(ARCH_DIR)/cpu/msr.c \
    $(ARCH_DIR)/cpu/syscall_hal.c \
    $(ARCH_DIR)/cpu/apic.c \
    $(ARCH_DIR)/mm/mmu.c \
    $(ARCH_DIR)/mm/uaccess.c \
    $(ARCH_DIR)/platform/platform.c \
    $(ARCH_DIR)/hal.c \
    $(ARCH_DIR)/virtio.c
else
HAL_SOURCES = \
    $(ARCH_DIR)/cpu/cpu.c \
    $(ARCH_DIR)/cpu/syscall.c \
    $(ARCH_DIR)/mm/mmu.c \
    $(ARCH_DIR)/platform.c \
    $(ARCH_DIR)/hal.c \
    $(ARCH_DIR)/virtio.c
endif

# --- Libkernel Sources (Architecture-Agnostic) ---
LIBKERNEL_SOURCES = \
    $(KERNEL_DIR)/libkernel/src/string.c \
    $(KERNEL_DIR)/libkernel/src/math.c \
    $(KERNEL_DIR)/libkernel/src/utf8.c \
    $(KERNEL_DIR)/libkernel/src/crc32.c \
    $(KERNEL_DIR)/libkernel/src/vsnprintf.c \
    $(KERNEL_DIR)/libkernel/src/printk.c \
    $(KERNEL_DIR)/libkernel/src/stack_protector.c \
    $(KERNEL_DIR)/libkernel/src/kmalloc.c \
    $(KERNEL_DIR)/libkernel/src/registry.c \
    $(KERNEL_DIR)/libkernel/src/fdt.c \
    $(KERNEL_DIR)/libkernel/src/vfs/vfs_init.c \
    $(KERNEL_DIR)/libkernel/src/vfs/vfs_subr.c \
    $(KERNEL_DIR)/libkernel/src/vfs/vfs_hash.c \
    $(KERNEL_DIR)/libkernel/src/vfs/vfs_cache.c \
    $(KERNEL_DIR)/libkernel/src/vfs/vfs_mount.c \
    $(KERNEL_DIR)/libkernel/src/vfs/vfs_lookup.c \
    $(KERNEL_DIR)/libkernel/src/vfs/vfs_file.c

# --- Microkernel Core Sources ---
CORE_SOURCES = \
    $(KERNEL_DIR)/core/src/main.c \
    $(KERNEL_DIR)/core/src/syscall.c \
    $(KERNEL_DIR)/core/src/stubs.c \
    $(KERNEL_DIR)/core/src/timer.c \
    $(KERNEL_DIR)/core/src/sched/process.c \
    $(KERNEL_DIR)/core/src/sched/elf.c \
    $(KERNEL_DIR)/core/src/syscall_proc.c \
    $(KERNEL_DIR)/core/src/cpu.c \
    $(KERNEL_DIR)/core/src/drivers.c \
    $(KERNEL_DIR)/core/src/bus.c \
    $(KERNEL_DIR)/core/src/fs/boot_fs.c \
    $(KERNEL_DIR)/core/src/gpu/graphics.c \
    $(KERNEL_DIR)/core/src/mm/pmm.c \
    $(KERNEL_DIR)/core/src/mm/vmm.c \
    $(KERNEL_DIR)/core/src/mm/buffer.c \
    $(KERNEL_DIR)/core/src/irq/irq.c

# --- Module Sources (pluggable VFS adaptors + graphics rendering) ---
MODULE_SOURCES = \
    $(KERNEL_DIR)/module/fs/vfs_ext4.c \
    $(KERNEL_DIR)/module/graphics/region.c \
    $(KERNEL_DIR)/module/graphics/gl.c \
    $(KERNEL_DIR)/module/graphics/font.c \
    $(KERNEL_DIR)/module/graphics/compositor.c

# --- Driver Sources (device drivers, formerly hal/drivers/) ---
ifeq ($(ARCH), amd64)
DRIVER_SOURCES = \
    $(KERNEL_DIR)/driver/console.c \
    $(KERNEL_DIR)/driver/irq_ctrl.c \
    $(KERNEL_DIR)/driver/sys_timer.c \
    $(KERNEL_DIR)/driver/mmio/uart/16550.c \
    $(KERNEL_DIR)/driver/mmio/timer/pic_pit.c \
    $(KERNEL_DIR)/driver/pci/core/pci.c \
    $(KERNEL_DIR)/driver/pci/storage/virtio_blk.c \
    $(KERNEL_DIR)/driver/pci/input/virtio_input.c \
    $(KERNEL_DIR)/driver/pci/graphics/virtio_gpu.c \
    $(KERNEL_DIR)/driver/pci/graphics/gpu_core.c \
    $(KERNEL_DIR)/driver/pci/input/keyboard.c
else
DRIVER_SOURCES = \
    $(KERNEL_DIR)/driver/console.c \
    $(KERNEL_DIR)/driver/irq_ctrl.c \
    $(KERNEL_DIR)/driver/sys_timer.c \
    $(KERNEL_DIR)/driver/mmio/uart/pl011.c \
    $(KERNEL_DIR)/driver/mmio/gic/gic.c \
    $(KERNEL_DIR)/driver/mmio/timer/timer.c \
    $(KERNEL_DIR)/driver/pci/storage/virtio_blk.c \
    $(KERNEL_DIR)/driver/pci/input/virtio_input.c \
    $(KERNEL_DIR)/driver/pci/graphics/virtio_gpu.c \
    $(KERNEL_DIR)/driver/pci/graphics/gpu_core.c \
    $(KERNEL_DIR)/driver/pci/input/keyboard.c
endif

KERN_C_SOURCES = $(HAL_SOURCES) $(LIBKERNEL_SOURCES) $(CORE_SOURCES) $(MODULE_SOURCES) $(DRIVER_SOURCES)

KERN_CPP_SOURCES = \
    $(KERNEL_DIR)/driver/cpp_test.cpp

# Object files
BOOT_OBJECTS = $(patsubst %.S,$(BUILD_DIR)/%.o,$(BOOT_SOURCES))
KERN_ASM_OBJECTS = $(KERN_ASM_SOURCES:%.S=$(BUILD_DIR)/%.o)
KERN_C_OBJECTS = $(KERN_C_SOURCES:%.c=$(BUILD_DIR)/%.o)
KERN_OBJECTS = $(KERN_ASM_OBJECTS) $(KERN_C_OBJECTS)

# Dependency files
DEPS = $(KERN_C_OBJECTS:.o=.d)

# ==============================================================================
# Build Rules
# ==============================================================================

.PHONY: all clean run run-direct debug disasm check dirs bootloader kernel disk rootfs release test-release help

# Default target
all: dirs bootloader kernel user $(MKDISK) disk
	@echo ""
	@echo "Build complete for $(ARCH)!"
	@echo "  Disk Image: $(DISK_IMG)"
ifeq ($(ARCH), amd64)
	@echo "  Kernel:     $(KERNEL_ELF)"
else
	@echo "  Kernel:     $(KERNEL_BIN) (AArch64 Raw Binary)"
endif

# Create build directories
dirs:
	@mkdir -p $(BUILD_DIR)/$(BOOT_DIR)
	@mkdir -p $(BUILD_DIR)/$(ARCH_DIR)/boot
	@mkdir -p $(BUILD_DIR)/$(ARCH_DIR)/cpu
	@mkdir -p $(BUILD_DIR)/$(ARCH_DIR)/mm
	@mkdir -p $(BUILD_DIR)/$(ARCH_DIR)/platform
	@mkdir -p $(BUILD_DIR)/$(ARCH_DIR)/drivers
	@mkdir -p $(BUILD_DIR)/$(KERNEL_DIR)/core/src/sched
	@mkdir -p $(BUILD_DIR)/$(KERNEL_DIR)/core/src/fs
	@mkdir -p $(BUILD_DIR)/$(KERNEL_DIR)/core/src/gpu
	@mkdir -p $(BUILD_DIR)/$(KERNEL_DIR)/core/src/mm
	@mkdir -p $(BUILD_DIR)/$(KERNEL_DIR)/core/src/irq
	@mkdir -p $(BUILD_DIR)/$(KERNEL_DIR)/module/fs
	@mkdir -p $(BUILD_DIR)/$(KERNEL_DIR)/module/graphics
	@mkdir -p $(BUILD_DIR)/$(KERNEL_DIR)/driver/mmio/uart
	@mkdir -p $(BUILD_DIR)/$(KERNEL_DIR)/driver/mmio/gic
	@mkdir -p $(BUILD_DIR)/$(KERNEL_DIR)/driver/mmio/timer
	@mkdir -p $(BUILD_DIR)/$(KERNEL_DIR)/driver/pci/core
	@mkdir -p $(BUILD_DIR)/$(KERNEL_DIR)/driver/pci/storage
	@mkdir -p $(BUILD_DIR)/$(KERNEL_DIR)/driver/pci/graphics
	@mkdir -p $(BUILD_DIR)/$(KERNEL_DIR)/driver/pci/input
	@mkdir -p $(BUILD_DIR)/$(KERNEL_DIR)/libkernel/src
	@mkdir -p $(BUILD_DIR)/$(KERNEL_DIR)/libkernel/src/vfs
	@mkdir -p $(BUILD_DIR)/$(USER_DIR)/lib
	@mkdir -p $(BUILD_DIR)/$(USER_DIR)/sys/lib
	@mkdir -p $(BUILD_DIR)/$(USER_DIR)/sys/bin
	@mkdir -p $(BUILD_DIR)/$(USER_DIR)/bin
	@mkdir -p $(BUILD_DIR)/$(USER_ARCH_DIR)

# Bootloader
bootloader: $(BOOTLOADER_BIN)

$(BOOTLOADER_ELF): $(BOOT_OBJECTS)
	@$(LD) $(LDFLAGS_BOOT) -o $@ $^

$(BOOTLOADER_BIN): $(BOOTLOADER_ELF)
	@$(OBJCOPY) -O binary $< $@

# Kernel
kernel: $(KERNEL_BIN)

$(KERNEL_ELF): $(KERN_OBJECTS)
	@$(LD) $(LDFLAGS_KERN) -o $@ $^

$(KERNEL_BIN): $(KERNEL_ELF)
	@$(OBJCOPY) -O binary $< $@

# Userland
USER_SYSCALL_O = $(BUILD_DIR)/$(USER_ARCH_DIR)/syscall.o
USER_LIB_O     = $(BUILD_DIR)/$(USER_SYS_DIR)/lib/lib.o
USER_MALLOC_O  = $(BUILD_DIR)/$(USER_SYS_DIR)/lib/malloc.o

# System ELFs (placed in /sys/bin)
SYS_ELFS = $(BUILD_DIR)/init.elf $(BUILD_DIR)/shell.elf $(BUILD_DIR)/notify_srv.elf \
           $(BUILD_DIR)/regedit.elf $(BUILD_DIR)/fontman.elf

# User ELFs (placed in /bin)
BIN_ELFS = $(BUILD_DIR)/counter.elf $(BUILD_DIR)/demo3d.elf $(BUILD_DIR)/ipc_send.elf \
           $(BUILD_DIR)/ipc_recv.elf $(BUILD_DIR)/crash.elf $(BUILD_DIR)/writetest.elf \
           $(BUILD_DIR)/input_test.elf

USER_ELFS = $(SYS_ELFS) $(BIN_ELFS)

user: $(USER_ELFS)

# User Compilations
$(BUILD_DIR)/$(USER_DIR)/lib/%.o: $(USER_DIR)/lib/%.c
	@mkdir -p $(dir $@)
	@$(CC) $(USER_CFLAGS) -c $< -o $@

$(BUILD_DIR)/$(USER_DIR)/sys/lib/%.o: $(USER_DIR)/sys/lib/%.c
	@mkdir -p $(dir $@)
	@$(CC) $(USER_CFLAGS) -c $< -o $@

$(BUILD_DIR)/$(USER_DIR)/bin/%.o: $(USER_DIR)/bin/%.c
	@mkdir -p $(dir $@)
	@$(CC) $(USER_CFLAGS) -c $< -o $@

$(BUILD_DIR)/$(USER_DIR)/sys/bin/%.o: $(USER_DIR)/sys/bin/%.c
	@mkdir -p $(dir $@)
	@$(CC) $(USER_CFLAGS) -c $< -o $@

# Explicit dependencies for each user ELF
$(BUILD_DIR)/init.elf: $(BUILD_DIR)/$(USER_DIR)/sys/bin/init.o $(USER_LIB_O) $(USER_SYSCALL_O) $(USER_MALLOC_O)
$(BUILD_DIR)/counter.elf: $(BUILD_DIR)/$(USER_DIR)/bin/counter.o $(USER_LIB_O) $(USER_SYSCALL_O) $(USER_MALLOC_O)
$(BUILD_DIR)/shell.elf: $(BUILD_DIR)/$(USER_DIR)/sys/bin/shell.o $(BUILD_DIR)/$(USER_DIR)/sys/bin/proce.o $(USER_LIB_O) $(USER_SYSCALL_O) $(USER_MALLOC_O)
$(BUILD_DIR)/demo3d.elf: $(BUILD_DIR)/$(USER_DIR)/bin/demo3d.o $(USER_LIB_O) $(USER_SYSCALL_O) $(USER_MALLOC_O)
$(BUILD_DIR)/ipc_send.elf: $(BUILD_DIR)/$(USER_DIR)/bin/ipc_send.o $(USER_LIB_O) $(USER_SYSCALL_O) $(USER_MALLOC_O)
$(BUILD_DIR)/ipc_recv.elf: $(BUILD_DIR)/$(USER_DIR)/bin/ipc_recv.o $(USER_LIB_O) $(USER_SYSCALL_O) $(USER_MALLOC_O)
$(BUILD_DIR)/notify_srv.elf: $(BUILD_DIR)/$(USER_DIR)/sys/bin/notification_server.o $(USER_LIB_O) $(USER_SYSCALL_O) $(USER_MALLOC_O)
$(BUILD_DIR)/crash.elf: $(BUILD_DIR)/$(USER_DIR)/bin/crash.o $(USER_LIB_O) $(USER_SYSCALL_O) $(USER_MALLOC_O)
$(BUILD_DIR)/regedit.elf: $(BUILD_DIR)/$(USER_DIR)/sys/bin/regedit.o $(USER_LIB_O) $(USER_SYSCALL_O) $(USER_MALLOC_O)
$(BUILD_DIR)/writetest.elf: $(BUILD_DIR)/$(USER_DIR)/bin/writetest.o $(USER_LIB_O) $(USER_SYSCALL_O) $(USER_MALLOC_O)
$(BUILD_DIR)/input_test.elf: $(BUILD_DIR)/$(USER_DIR)/bin/input_test.o $(USER_LIB_O) $(USER_SYSCALL_O) $(USER_MALLOC_O)
$(BUILD_DIR)/fontman.elf: $(BUILD_DIR)/$(USER_DIR)/sys/bin/fontman/fontman.o $(USER_LIB_O) $(USER_SYSCALL_O) $(USER_MALLOC_O)

$(BUILD_DIR)/$(USER_DIR)/sys/bin/fontman/%.o: $(USER_DIR)/sys/bin/fontman/%.c
	@mkdir -p $(dir $@)
	@$(CC) $(USER_CFLAGS) -c $< -o $@


# Linking rule for user ELFs
$(BUILD_DIR)/%.elf:
	@$(CC) $(USER_CFLAGS) -Wl,-Ttext=0x80000000 -e _start -o $@ $^

# Common compilation rules
$(BUILD_DIR)/%.o: %.S
	@mkdir -p $(dir $@)
	@$(CC) $(CFLAGS) -c -o $@ $<

$(BUILD_DIR)/kernel/%.o: kernel/%.c
	@mkdir -p $(dir $@)
	@$(CC) $(CFLAGS) -DKERNEL -MMD -MP -c -o $@ $<

$(BUILD_DIR)/%.o: %.c
	@mkdir -p $(dir $@)
	@$(CC) $(CFLAGS) -MMD -MP -c -o $@ $<

$(BUILD_DIR)/%.o: %.cpp
	@mkdir -p $(dir $@)
	@$(CXX) $(CXXFLAGS) -MMD -MP -c -o $@ $<

-include $(DEPS)

# Disk Generation
rootfs: user
	@rm -rf $(BUILD_DIR)/rootfs
	@mkdir -p $(BUILD_DIR)/rootfs/sys/bin
	@mkdir -p $(BUILD_DIR)/rootfs/bin
	@mkdir -p $(BUILD_DIR)/rootfs/etc
	@mkdir -p $(BUILD_DIR)/rootfs/sys/lib
	@mkdir -p $(BUILD_DIR)/rootfs/lib
	@cp $(SYS_ELFS) $(BUILD_DIR)/rootfs/sys/bin/
	@cp $(BIN_ELFS) $(BUILD_DIR)/rootfs/bin/
	@cp user/sys/bin/init.cfg $(BUILD_DIR)/rootfs/etc/

	@mkdir -p $(BUILD_DIR)/rootfs/fonts
	@-cp user/sys/bin/fontman/fonts/*.ttf $(BUILD_DIR)/rootfs/fonts/ 2>/dev/null || true
	@-cp user/sys/bin/fontman/fonts/*.off $(BUILD_DIR)/rootfs/fonts/ 2>/dev/null || true
	@# Remove .elf extensions in rootfs
	@for f in $(BUILD_DIR)/rootfs/sys/bin/*.elf; do mv "$$f" "$${f%.elf}"; done
	@for f in $(BUILD_DIR)/rootfs/bin/*.elf; do mv "$$f" "$${f%.elf}"; done

disk: $(MKDISK) kernel rootfs bootloader
	@mkdir -p $(BUILD_DIR)
	@./$(MKDISK) $(DISK_IMG) $(BOOTLOADER_BIN) $(KERNEL_BIN) $(BUILD_DIR)/rootfs

$(MKDISK): tools/mkdisk.c
	@mkdir -p tools
	@gcc -o $(MKDISK) tools/mkdisk.c

# ==============================================================================
# QEMU Flags
# ==============================================================================

ifeq ($(ARCH), amd64)
QEMU_FLAGS = -m 3G -smp 4 -serial mon:stdio \
             -display default,show-cursor=on \
             -device virtio-gpu-pci,disable-legacy=on,disable-modern=off \
             -device virtio-keyboard-pci,disable-legacy=on,disable-modern=off \
             -device virtio-mouse-pci,disable-legacy=on,disable-modern=off \
             -drive if=none,file=$(DISK_IMG),id=hd0,format=raw \
             -device virtio-blk-pci,drive=hd0,disable-legacy=on,disable-modern=off

QEMU_RELEASE_FLAGS = -m 3G -smp 4 -serial mon:stdio \
                     -display default,show-cursor=on \
                     -device virtio-gpu-pci,disable-legacy=on,disable-modern=off \
                     -device virtio-keyboard-pci,disable-legacy=on,disable-modern=off \
                     -device virtio-mouse-pci,disable-legacy=on,disable-modern=off
else
QEMU_FLAGS = -M virt -cpu cortex-a57 -m 3G -smp 4 -serial mon:stdio \
             -display default,show-cursor=on \
             -device virtio-gpu-device \
             -device virtio-keyboard-device -device virtio-mouse-device \
             -drive if=none,file=$(DISK_IMG),id=hd0,format=raw -device virtio-blk-device,drive=hd0

QEMU_RELEASE_FLAGS = -M virt -cpu cortex-a57 -m 3G -smp 4 -serial mon:stdio \
                     -display default,show-cursor=on \
                     -device virtio-gpu-device \
                     -device virtio-keyboard-device -device virtio-mouse-device \
                     -drive if=none,file=$(RELEASE_DIR)/disk.img,id=hd0,format=raw -device virtio-blk-device,drive=hd0
endif

# ==============================================================================
# Release Generation
# ==============================================================================

release:
	@echo "========================================"
	@echo "  Creating Full Release $(VERSION)"
	@echo "========================================"
	@$(MAKE) ARCH=amd64 release-arch VERSION=$(VERSION)
	@$(MAKE) ARCH=aarch64 release-arch VERSION=$(VERSION)
	@echo "========================================"
	@echo "✓ Full release $(VERSION) complete!"
	@echo "  Location: $(RELEASE_BASE)"
	@echo "========================================"

release-arch: all
	@echo "--> Building $(ARCH) release..."
	@rm -rf $(RELEASE_DIR)
	@mkdir -p $(RELEASE_DIR)
ifeq ($(ARCH), amd64)
	@rm -rf $(RELEASE_DIR)/iso
	@mkdir -p $(RELEASE_DIR)/iso/boot/grub
	
	@cp $(KERNEL_ELF) $(RELEASE_DIR)/iso/boot/kernel.elf
	
	@echo 'set timeout=0' > $(RELEASE_DIR)/iso/boot/grub/grub.cfg
	@echo 'set default=0' >> $(RELEASE_DIR)/iso/boot/grub/grub.cfg
	@echo 'menuentry "OS1Test" {' >> $(RELEASE_DIR)/iso/boot/grub/grub.cfg
	@echo '  multiboot2 /boot/kernel.elf' >> $(RELEASE_DIR)/iso/boot/grub/grub.cfg
	@echo '  boot' >> $(RELEASE_DIR)/iso/boot/grub/grub.cfg
	@echo '}' >> $(RELEASE_DIR)/iso/boot/grub/grub.cfg
	
	@dd if=$(DISK_IMG) of=$(BUILD_DIR)/userland.img bs=512 skip=34850 status=none
	
	$(GRUB_MKRESCUE) -o $(RELEASE_DIR)/os1test-amd64-$(VERSION).iso $(RELEASE_DIR)/iso \
		-- -append_partition 2 0x83 $(BUILD_DIR)/userland.img
	
	@echo "✓ AMD64 Hybrid ISO: $(RELEASE_DIR)/os1test-amd64-$(VERSION).iso"
else
	@cp $(KERNEL_BIN) $(RELEASE_DIR)/kernel.img
	@cp $(DISK_IMG) $(RELEASE_DIR)/disk.img
	@echo "✓ AArch64 release files: kernel.img, disk.img"
endif

test-release: release-arch
	@echo "Starting QEMU Release Test for $(ARCH) (Version: $(VERSION))..."
ifeq ($(ARCH), amd64)
	$(QEMU) $(QEMU_RELEASE_FLAGS) \
		-drive if=none,file=$(RELEASE_DIR)/os1test-amd64-$(VERSION).iso,id=hd0,format=raw \
		-device virtio-blk-pci,drive=hd0,disable-legacy=on,disable-modern=off
else
	$(QEMU) $(QEMU_RELEASE_FLAGS) -kernel $(RELEASE_DIR)/kernel.img
endif

# ==============================================================================
# Development Execution
# ==============================================================================

run: all
	$(QEMU) $(QEMU_FLAGS) -kernel $(KERNEL_ELF)

run-direct: run

debug: all
ifeq ($(ARCH), amd64)
	$(QEMU) $(QEMU_FLAGS) -kernel $(KERNEL_ELF) -s -S
else
	$(QEMU) $(QEMU_FLAGS) -kernel $(KERNEL_ELF) -s -S
endif

disasm: $(KERNEL_ELF) $(BOOTLOADER_ELF)
	$(OBJDUMP) -d $(KERNEL_ELF) > $(BUILD_DIR)/kernel.disasm
	$(OBJDUMP) -d $(BOOTLOADER_ELF) > $(BUILD_DIR)/bootloader.disasm
	$(OBJDUMP) -d $(USER_ELF) > $(BUILD_DIR)/init.disasm

check:
	@which $(CC) > /dev/null && echo "✓ CC: $(CC)" || echo "✗ CC not found"
	@which $(AS) > /dev/null && echo "✓ AS: $(AS)" || echo "✗ AS not found"
	@which $(LD) > /dev/null && echo "✓ LD: $(LD)" || echo "✗ LD not found"
	@which $(QEMU) > /dev/null && echo "✓ QEMU: $(QEMU)" || echo "✗ QEMU not found"
	@if [ -n "$(GRUB_MKRESCUE)" ]; then echo "✓ GRUB: $(GRUB_MKRESCUE)"; else echo "✗ GRUB mancante"; fi

clean:
	-@rm -rf build/aarch64 2>/dev/null || true
	-@rm -rf build/amd64 2>/dev/null || true
	-@rm -rf build/*.img build/*.iso 2>/dev/null || true
	-@rm -rf $(BUILD_ROOT) 2>/dev/null || true
	-@rm -rf release 2>/dev/null || true
	-@rm $(MKDISK) 2>/dev/null || true

help:
	@echo "$(ARCH) Kernel Build System"
	@echo "Targets:"
	@echo "  all          - Build bootloader, kernel, and disk image"
	@echo "  release      - Build production files (Use: make release VERSION=0.1.2)"
	@echo "  test-release - Build release and test it in QEMU (Use: make test-release VERSION=0.1.2)"
	@echo "  run          - Build and run kernel directly"
	@echo "  clean        - Remove build artifacts"
