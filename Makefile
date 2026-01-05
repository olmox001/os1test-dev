# Makefile for AArch64 Production Kernel
# Cross-compilation for ARM 64-bit bare-metal

# ==============================================================================
# Configuration
# ==============================================================================

# Cross-compiler prefix
CROSS_COMPILE ?= aarch64-none-elf-

# Tools
CC      = $(CROSS_COMPILE)gcc
AS      = $(CROSS_COMPILE)as
LD      = $(CROSS_COMPILE)ld
OBJCOPY = $(CROSS_COMPILE)objcopy
OBJDUMP = $(CROSS_COMPILE)objdump

# Directories
KERNEL_DIR = kernel
BOOT_DIR   = boot
ARCH_DIR   = $(KERNEL_DIR)/arch/aarch64
BUILD_DIR  = build
INCLUDE    = -I$(KERNEL_DIR)/include

# Output files
BOOTLOADER_ELF = $(BUILD_DIR)/bootloader.elf
BOOTLOADER_BIN = $(BUILD_DIR)/bootloader.bin
KERNEL_ELF = $(BUILD_DIR)/kernel.elf
KERNEL_ELF = $(BUILD_DIR)/kernel.elf
KERNEL_BIN = $(BUILD_DIR)/kernel.bin
USER_ELF   = $(BUILD_DIR)/init.elf


# Compiler flags
CFLAGS = -Wall -Wextra -Werror \
         -ffreestanding -fno-builtin -nostdlib -nostartfiles \
         -mcpu=cortex-a57 \
         -fno-common -fno-stack-protector \
         -fno-pic -fno-pie \
         -fno-omit-frame-pointer \
         -O2 -g \
         $(INCLUDE)

# Assembler flags
ASFLAGS = -mcpu=cortex-a57 -g

# Linker flags
LDFLAGS_BOOT = -nostdlib -static -T $(BOOT_DIR)/linker.ld
LDFLAGS_KERN = -nostdlib -static -T $(KERNEL_DIR)/kernel.ld

# ==============================================================================
# Source Files
# ==============================================================================

# Bootloader sources
BOOT_SOURCES = \
    $(BOOT_DIR)/header.S \
    $(BOOT_DIR)/stage1.S \
    $(BOOT_DIR)/stage2.S

# Kernel assembly sources
KERN_ASM_SOURCES = \
    $(ARCH_DIR)/boot/start.S \
    $(ARCH_DIR)/cpu/exception.S

KERN_C_SOURCES = \
    $(ARCH_DIR)/cpu/cpu.c \
    $(ARCH_DIR)/cpu/syscall.c \
    $(KERNEL_DIR)/drivers/uart/pl011.c \
    $(KERNEL_DIR)/drivers/gic/gic.c \
    $(KERNEL_DIR)/drivers/timer/timer.c \
    $(KERNEL_DIR)/drivers/virtio/virtio_blk.c \
    $(KERNEL_DIR)/drivers/virtio/virtio_input.c \
    $(KERNEL_DIR)/drivers/gpu/virtio_gpu.c \
    $(KERNEL_DIR)/drivers/keyboard/keyboard.c \
    $(KERNEL_DIR)/fs/gpt.c \
    $(KERNEL_DIR)/fs/ext4.c \
    $(KERNEL_DIR)/mm/pmm.c \
    $(KERNEL_DIR)/mm/vmm.c \
    $(KERNEL_DIR)/mm/buffer.c \
    $(KERNEL_DIR)/lib/string.c \
    $(KERNEL_DIR)/lib/printk.c \
    $(KERNEL_DIR)/lib/math.c \
    $(KERNEL_DIR)/lib/kmalloc.c \
    $(KERNEL_DIR)/sched/process.c \
    $(KERNEL_DIR)/sched/elf.c \
    $(KERNEL_DIR)/graphics/graphics.c \
    $(KERNEL_DIR)/graphics/font.c \
    $(KERNEL_DIR)/graphics/draw2d.c \
    $(KERNEL_DIR)/graphics/draw3d.c \
    $(KERNEL_DIR)/graphics/compositor.c \
    $(KERNEL_DIR)/kernel.c


# Object files
BOOT_OBJECTS = $(BOOT_SOURCES:%.S=$(BUILD_DIR)/%.o)
KERN_ASM_OBJECTS = $(KERN_ASM_SOURCES:%.S=$(BUILD_DIR)/%.o)
KERN_C_OBJECTS = $(KERN_C_SOURCES:%.c=$(BUILD_DIR)/%.o)
KERN_OBJECTS = $(KERN_ASM_OBJECTS) $(KERN_C_OBJECTS)

# Tools
MKDISK = tools/mkdisk

# Dependency files
DEPS = $(KERN_C_OBJECTS:.o=.d)

# ==============================================================================
# Build Rules
# ==============================================================================

.PHONY: all clean run run-direct debug disasm check dirs bootloader kernel iso

# Default target - builds everything including ISO
all: dirs bootloader kernel user $(MKDISK) disk
	@echo ""
	@echo "Build complete!"

	@echo "  Bootloader: $(BOOTLOADER_ELF)"
	@echo "  Kernel:     $(KERNEL_ELF)"
	@echo ""
	@echo "Run with: make run  (or ./build_iso.sh run)"
	@echo ""

# Create build directories
dirs:
	@mkdir -p $(BUILD_DIR)/$(BOOT_DIR)
	@mkdir -p $(BUILD_DIR)/$(ARCH_DIR)/boot
	@mkdir -p $(BUILD_DIR)/$(ARCH_DIR)/cpu
	@mkdir -p $(BUILD_DIR)/$(ARCH_DIR)/mm
	@mkdir -p $(BUILD_DIR)/$(KERNEL_DIR)/drivers/uart
	@mkdir -p $(BUILD_DIR)/$(KERNEL_DIR)/drivers/gic
	@mkdir -p $(BUILD_DIR)/$(KERNEL_DIR)/drivers/timer
	@mkdir -p $(BUILD_DIR)/$(KERNEL_DIR)/drivers/virtio
	@mkdir -p $(BUILD_DIR)/$(KERNEL_DIR)/drivers/gpu
	@mkdir -p $(BUILD_DIR)/$(KERNEL_DIR)/drivers/keyboard
	@mkdir -p $(BUILD_DIR)/$(KERNEL_DIR)/fs
	@mkdir -p $(BUILD_DIR)/$(KERNEL_DIR)/mm
	@mkdir -p $(BUILD_DIR)/$(KERNEL_DIR)/lib
	@mkdir -p $(BUILD_DIR)/$(KERNEL_DIR)/sched
	@mkdir -p $(BUILD_DIR)/$(KERNEL_DIR)/graphics
	@mkdir -p $(BUILD_DIR)/user


# ==============================================================================
# Bootloader
# ==============================================================================

bootloader: $(BOOTLOADER_BIN)

$(BOOTLOADER_ELF): $(BOOT_OBJECTS)
	@echo "Linking bootloader..."
	$(LD) $(LDFLAGS_BOOT) -o $@ $^
	@echo "Bootloader size: $$(stat -f%z $@ 2>/dev/null || stat -c%s $@ 2>/dev/null) bytes"

$(BOOTLOADER_BIN): $(BOOTLOADER_ELF)
	@echo "Creating bootloader binary..."
	$(OBJCOPY) -O binary $< $@

# ==============================================================================
# Kernel
# ==============================================================================

kernel: $(KERNEL_BIN)

$(KERNEL_ELF): $(KERN_OBJECTS)
	@echo "Linking kernel..."
	$(LD) $(LDFLAGS_KERN) -o $@ $^
	@echo "Kernel size: $$(stat -f%z $@ 2>/dev/null || stat -c%s $@ 2>/dev/null) bytes"

$(KERNEL_BIN): $(KERNEL_ELF)
	@echo "Creating kernel binary..."
	$(OBJCOPY) -O binary $< $@

# ==============================================================================
# Userland
# ==============================================================================

user: $(USER_ELF) $(BUILD_DIR)/counter.elf $(BUILD_DIR)/shell.elf

# User Init
USER_SRC = user/init.c user/lib.c
USER_OBJ = $(USER_SRC:%.c=$(BUILD_DIR)/%.o)

# User Counter
COUNTER_SRC = user/counter.c user/lib.c
COUNTER_OBJ = $(COUNTER_SRC:%.c=$(BUILD_DIR)/%.o)

# User Shell
SHELL_SRC = user/shell.c user/lib.c
SHELL_OBJ = $(SHELL_SRC:%.c=$(BUILD_DIR)/%.o)

$(BUILD_DIR)/user/%.o: user/%.c
	@mkdir -p $(dir $@)
	$(CC) $(CFLAGS) -c $< -o $@

$(USER_ELF): $(USER_OBJ)
	$(CC) $(CFLAGS) -Ttext=0x80000000 -e main -o $@ $^
	@echo "Userland init size: $$(stat -f%z $@ 2>/dev/null || stat -c%s $@ 2>/dev/null) bytes"

$(BUILD_DIR)/counter.elf: $(COUNTER_OBJ)
	$(CC) $(CFLAGS) -Ttext=0x80000000 -e main -o $@ $^
	@echo "Userland counter size: $$(stat -f%z $@ 2>/dev/null || stat -c%s $@ 2>/dev/null) bytes"

$(BUILD_DIR)/shell.elf: $(SHELL_OBJ)
	$(CC) $(CFLAGS) -Ttext=0x80000000 -e main -o $@ $^
	@echo "Userland shell size: $$(stat -f%z $@ 2>/dev/null || stat -c%s $@ 2>/dev/null) bytes"



# ==============================================================================
# Common compilation rules

# ==============================================================================

# Compile assembly files
$(BUILD_DIR)/%.o: %.S
	@echo "AS  $<"
	@$(AS) $(ASFLAGS) -o $@ $<

# Compile C files
$(BUILD_DIR)/%.o: %.c
	@echo "CC  $<"
	@$(CC) $(CFLAGS) -MMD -MP -c -o $@ $<

# Include dependencies
-include $(DEPS)

# ==============================================================================
# Run and Debug
# ==============================================================================

QEMU = qemu-system-aarch64
QEMU_FLAGS = -M virt -cpu cortex-a57 -m 1G -smp 4 -nographic -serial mon:stdio \
             -device virtio-keyboard-device -device virtio-mouse-device

# Run with ISO boot (full boot chain)
run: all
	@./build_iso.sh run

# Run with direct kernel boot (faster, skips bootloader/ISO)
run-direct: kernel
	@echo ""
	@echo "Starting QEMU (direct kernel boot)..."
	@echo "Press Ctrl+C to exit"
	@echo ""
	$(QEMU) $(QEMU_FLAGS) -kernel $(KERNEL_ELF)

# Debug with QEMU and GDB
debug: all
	@./build_iso.sh debug

# ==============================================================================
# Analysis Tools
# ==============================================================================

disasm: $(KERNEL_ELF) $(BOOTLOADER_ELF)
	$(OBJDUMP) -d $(KERNEL_ELF) > $(BUILD_DIR)/kernel.disasm
	$(OBJDUMP) -d $(BOOTLOADER_ELF) > $(BUILD_DIR)/bootloader.disasm
	$(OBJDUMP) -d $(USER_ELF) > $(BUILD_DIR)/init.disasm
	@echo "Disassembly saved to $(BUILD_DIR)/"

check:
	@echo "Checking toolchain..."
	@which $(CC) > /dev/null && echo "✓ CC: $(CC)" || echo "✗ CC not found"
	@which $(AS) > /dev/null && echo "✓ AS: $(AS)" || echo "✗ AS not found"
	@which $(LD) > /dev/null && echo "✓ LD: $(LD)" || echo "✗ LD not found"
	@which $(QEMU) > /dev/null && echo "✓ QEMU: $(QEMU)" || echo "✗ QEMU not found"

# ==============================================================================
# Clean
# ==============================================================================

disk: $(MKDISK) kernel user bootloader
	@echo "Creating disk image..."
	@mkdir -p $(BUILD_DIR)
	@./$(MKDISK) $(BUILD_DIR)/disk.img

$(MKDISK): tools/mkdisk.c
	@mkdir -p tools
	@echo "CC  tools/mkdisk.c"
	@gcc -o $(MKDISK) tools/mkdisk.c

clean:
	rm -rf $(BUILD_DIR)
	@echo "Build directory cleaned"

# ==============================================================================
# Help
# ==============================================================================

help:
	@echo "AArch64 Kernel Build System"
	@echo ""
	@echo "Targets:"
	@echo "  all        - Build bootloader and kernel (default)"
	@echo "  bootloader - Build only bootloader"
	@echo "  kernel     - Build only kernel"
	@echo "  run        - Build and run with ISO boot"
	@echo "  run-direct - Build and run kernel directly"
	@echo "  debug      - Run with GDB debug"
	@echo "  disasm     - Generate disassembly"
	@echo "  clean      - Remove build artifacts"
	@echo "  check      - Verify toolchain"
	@echo "  help       - Show this help"
