#!/bin/bash
#
# build_iso.sh - Script completo per creare un'immagine bootabile AArch64
#
# Questo script:
# 1. Verifica e installa le dipendenze necessarie
# 2. Compila bootloader e kernel
# 3. Scarica/compila GRUB EFI per AArch64 se necessario
# 4. Crea un'immagine ISO/IMG bootabile
#
# Uso: ./build_iso.sh [clean|build|iso|run|all]
#

set -e

# Configurazione
PROJECT_DIR="$(cd "$(dirname "$0")" && pwd)"
BUILD_DIR="${PROJECT_DIR}/build"
ISO_DIR="${BUILD_DIR}/iso"
EFI_DIR="${ISO_DIR}/EFI/BOOT"
DEPS_DIR="${PROJECT_DIR}/deps"

# Colori per output
RED='\033[0;31m'
GREEN='\033[0;32m'
YELLOW='\033[1;33m'
NC='\033[0m' # No Color

log_info() { echo -e "${GREEN}[INFO]${NC} $1"; }
log_warn() { echo -e "${YELLOW}[WARN]${NC} $1"; }
log_error() { echo -e "${RED}[ERROR]${NC} $1"; }

# ============================================================================
# VERIFICA DIPENDENZE
# ============================================================================
check_dependencies() {
    log_info "Verificando dipendenze..."
    
    local missing=()
    
    # Toolchain AArch64
    if ! command -v aarch64-none-elf-as &> /dev/null; then
        missing+=("aarch64-none-elf toolchain")
    fi
    
    # QEMU
    if ! command -v qemu-system-aarch64 &> /dev/null; then
        missing+=("qemu")
    fi
    
    # mtools per creare immagini FAT
    if ! command -v mformat &> /dev/null; then
        missing+=("mtools")
    fi
    
    # xorriso per creare ISO
    if ! command -v xorriso &> /dev/null; then
        missing+=("xorriso")
    fi
    
    if [ ${#missing[@]} -gt 0 ]; then
        log_warn "Dipendenze mancanti: ${missing[*]}"
        log_info "Installando dipendenze..."
        install_dependencies
    else
        log_info "Tutte le dipendenze sono presenti"
    fi
}

install_dependencies() {
    if [[ "$OSTYPE" == "darwin"* ]]; then
        # macOS
        log_info "Rilevato macOS, usando Homebrew..."
        
        # Tap per cross-compiler
        if ! brew tap | grep -q "sergiobenitez/osxct"; then
            brew tap SergioBenitez/osxct
        fi
        
        # Installa toolchain se mancante
        if ! command -v aarch64-none-elf-as &> /dev/null; then
            brew install aarch64-none-elf
        fi
        
        # Installa QEMU se mancante
        if ! command -v qemu-system-aarch64 &> /dev/null; then
            brew install qemu
        fi
        
        # Installa mtools per immagini FAT
        if ! command -v mformat &> /dev/null; then
            brew install mtools
        fi
        
        # Installa xorriso per ISO
        if ! command -v xorriso &> /dev/null; then
            brew install xorriso
        fi
        
    elif [[ "$OSTYPE" == "linux-gnu"* ]]; then
        # Linux
        log_info "Rilevato Linux..."
        if command -v apt-get &> /dev/null; then
            sudo apt-get update
            sudo apt-get install -y gcc-aarch64-linux-gnu qemu-system-arm mtools xorriso
        elif command -v pacman &> /dev/null; then
            sudo pacman -S aarch64-linux-gnu-gcc qemu-system-aarch64 mtools xorriso
        fi
    fi
}

# ============================================================================
# DOWNLOAD GRUB EFI PER AARCH64
# ============================================================================
download_grub_efi() {
    log_info "Preparando GRUB EFI per AArch64..."
    
    mkdir -p "${DEPS_DIR}"
    
    local GRUB_EFI="${DEPS_DIR}/BOOTAA64.EFI"
    
    if [ -f "${GRUB_EFI}" ]; then
        log_info "GRUB EFI già presente"
        return 0
    fi
    
    # Prova a scaricare GRUB precompilato da Debian/Ubuntu
    log_info "Scaricando GRUB EFI precompilato..."
    
    local GRUB_URL="http://ftp.debian.org/debian/pool/main/g/grub-efi-arm64-signed/grub-efi-arm64-signed_1+2.06+13+deb12u1_arm64.deb"
    local GRUB_DEB="${DEPS_DIR}/grub-efi.deb"
    
    if curl -L -o "${GRUB_DEB}" "${GRUB_URL}" 2>/dev/null; then
        log_info "Estraendo GRUB EFI..."
        cd "${DEPS_DIR}"
        ar x grub-efi.deb 2>/dev/null || true
        tar xf data.tar.* 2>/dev/null || true
        
        # Cerca il file EFI
        local found_efi=$(find . -name "grubaa64.efi" -o -name "grubnetaa64.efi" 2>/dev/null | head -1)
        if [ -n "${found_efi}" ]; then
            cp "${found_efi}" "${GRUB_EFI}"
            log_info "GRUB EFI estratto con successo"
        fi
        
        # Cleanup
        rm -f grub-efi.deb control.tar.* data.tar.* debian-binary
        rm -rf usr
        cd "${PROJECT_DIR}"
    fi
    
    # Se il download fallisce, creiamo un EFI stub minimale
    if [ ! -f "${GRUB_EFI}" ]; then
        log_warn "Download GRUB fallito, creando EFI stub minimale..."
        create_efi_stub
    fi
}

# ============================================================================
# CREA EFI STUB MINIMALE (alternativa a GRUB)
# ============================================================================
create_efi_stub() {
    log_info "Creando EFI stub loader..."
    
    mkdir -p "${BUILD_DIR}"
    
    cat > "${PROJECT_DIR}/boot/efi_stub.S" << 'EOF'
// EFI Stub minimale per AArch64
// Questo stub viene caricato da UEFI e salta al kernel

.section .text
.global _start

// EFI entry point
// x0 = EFI_HANDLE
// x1 = EFI_SYSTEM_TABLE*
_start:
    // Salva parametri EFI
    mov x19, x0
    mov x20, x1
    
    // Disabilita interrupts
    msr daifset, #0xf
    
    // Setup stack
    adrp x0, __stack_top
    add x0, x0, :lo12:__stack_top
    mov sp, x0
    
    // Chiama EFI Exit Boot Services prima di prendere controllo
    // Per ora, saltiamo direttamente al kernel
    
    // Carica kernel address
    adrp x0, kernel_load_addr
    add x0, x0, :lo12:kernel_load_addr
    ldr x0, [x0]
    
    // Se kernel_load_addr è 0, usa indirizzo di default
    cbz x0, use_default_kernel
    br x0

use_default_kernel:
    // Salta a kernel entry di default
    ldr x0, =0x40400000
    br x0

.section .data
.align 8
kernel_load_addr:
    .quad 0

.section .bss
.align 16
__efi_stack:
    .skip 65536
__stack_top:
EOF

    # Compila EFI stub
    aarch64-none-elf-as -o "${BUILD_DIR}/efi_stub.o" "${PROJECT_DIR}/boot/efi_stub.S"
    
    # Crea linker script per EFI
    cat > "${BUILD_DIR}/efi.ld" << 'EOF'
ENTRY(_start)
SECTIONS {
    . = 0x0;
    .text : { *(.text) }
    .data : { *(.data) }
    .bss : { *(.bss) }
}
EOF

    aarch64-none-elf-ld -T "${BUILD_DIR}/efi.ld" -o "${BUILD_DIR}/efi_stub.elf" "${BUILD_DIR}/efi_stub.o"
    aarch64-none-elf-objcopy -O binary "${BUILD_DIR}/efi_stub.elf" "${DEPS_DIR}/BOOTAA64.EFI"
    
    log_info "EFI stub creato"
}

# ============================================================================
# COMPILA BOOTLOADER E KERNEL
# ============================================================================
build_bootloader() {
    log_info "Compilando bootloader e kernel..."
    
    mkdir -p "${BUILD_DIR}"
    
    cd "${PROJECT_DIR}"
    make clean
    make all
    
    # kernel.elf e kernel.bin sono già in build/ dal Makefile
    
    log_info "Compilazione completata"
}

# ============================================================================
# CREA CONFIGURAZIONE GRUB
# ============================================================================
create_grub_config() {
    log_info "Creando configurazione GRUB..."
    
    mkdir -p "${ISO_DIR}/boot/grub"
    
    cat > "${ISO_DIR}/boot/grub/grub.cfg" << 'EOF'
# GRUB Configuration per AArch64 Bootloader

set timeout=5
set default=0

# Menu principale
menuentry "AArch64 OS Bootloader" {
    echo "Caricando bootloader..."
    multiboot2 /boot/bootloader.bin
    echo "Caricando kernel..."
    module2 /boot/kernel.bin
    boot
}

menuentry "AArch64 OS (modalità debug)" {
    echo "Avvio in modalità debug..."
    set debug=all
    multiboot2 /boot/bootloader.bin debug
    module2 /boot/kernel.bin
    boot
}

menuentry "Reboot" {
    reboot
}

menuentry "Poweroff" {
    halt
}
EOF

    log_info "Configurazione GRUB creata"
}

# ============================================================================
# CREA IMMAGINE EFI BOOTABILE
# ============================================================================
create_efi_image() {
    log_info "Creando immagine EFI bootabile..."
    
    local EFI_IMG="${BUILD_DIR}/efi.img"
    local IMG_SIZE=64  # MB
    
    # Crea immagine FAT32 vuota
    dd if=/dev/zero of="${EFI_IMG}" bs=1M count=${IMG_SIZE} 2>/dev/null
    
    # Formatta come FAT32
    mformat -i "${EFI_IMG}" -F ::
    
    # Crea struttura directory
    mmd -i "${EFI_IMG}" ::/EFI
    mmd -i "${EFI_IMG}" ::/EFI/BOOT
    mmd -i "${EFI_IMG}" ::/boot
    mmd -i "${EFI_IMG}" ::/boot/grub
    
    # Copia file EFI
    if [ -f "${DEPS_DIR}/BOOTAA64.EFI" ]; then
        mcopy -i "${EFI_IMG}" "${DEPS_DIR}/BOOTAA64.EFI" ::/EFI/BOOT/
    fi
    
    # Copia bootloader e kernel
    mcopy -i "${EFI_IMG}" "${BUILD_DIR}/bootloader.bin" ::/boot/
    mcopy -i "${EFI_IMG}" "${BUILD_DIR}/kernel.bin" ::/boot/
    
    # Copia configurazione GRUB
    if [ -f "${ISO_DIR}/boot/grub/grub.cfg" ]; then
        mcopy -i "${EFI_IMG}" "${ISO_DIR}/boot/grub/grub.cfg" ::/boot/grub/
    fi
    
    log_info "Immagine EFI creata: ${EFI_IMG}"
}

# ============================================================================
# CREA ISO BOOTABILE
# ============================================================================
create_iso() {
    log_info "Creando ISO bootabile..."
    
    local ISO_FILE="${BUILD_DIR}/aarch64-os.iso"
    
    # Prepara struttura ISO
    mkdir -p "${EFI_DIR}"
    mkdir -p "${ISO_DIR}/boot"
    
    # Copia file boot
    cp "${BUILD_DIR}/kernel.bin" "${ISO_DIR}/boot/"
    
    # Copia EFI loader
    if [ -f "${DEPS_DIR}/BOOTAA64.EFI" ]; then
        cp "${DEPS_DIR}/BOOTAA64.EFI" "${EFI_DIR}/"
    fi
    
    # Crea immagine EFI per embedding in ISO
    create_efi_image
    
    # Crea ISO con xorriso
    if command -v xorriso &> /dev/null; then
        xorriso -as mkisofs \
            -o "${ISO_FILE}" \
            -e build/efi.img \
            -no-emul-boot \
            -isohybrid-gpt-basdat \
            "${ISO_DIR}" 2>/dev/null || {
            # Fallback: crea ISO semplice
            log_warn "xorriso fallback a metodo semplice..."
            xorriso -as mkisofs -o "${ISO_FILE}" "${ISO_DIR}" 2>/dev/null || true
        }
    fi
    
    if [ -f "${ISO_FILE}" ]; then
        log_info "ISO creata: ${ISO_FILE}"
        log_info "Dimensione: $(du -h "${ISO_FILE}" | cut -f1)"
    else
        log_warn "ISO non creata, usando immagine EFI direttamente"
    fi
}

# ============================================================================
# CREA IMMAGINE DISCO DIRETTAMENTE BOOTABILE
# ============================================================================
create_disk_image() {
    log_info "Creando immagine disco bootabile..."
    
    local DISK_IMG="${BUILD_DIR}/aarch64-os.img"
    local IMG_SIZE=128  # MB
    
    # Crea immagine disco vuota
    dd if=/dev/zero of="${DISK_IMG}" bs=1M count=${IMG_SIZE} 2>/dev/null
    
    # Crea tabella partizioni GPT con una partizione EFI
    # Usiamo fdisk se disponibile, altrimenti scriviamo direttamente
    if command -v sgdisk &> /dev/null; then
        sgdisk -Z "${DISK_IMG}" 2>/dev/null || true
        sgdisk -n 1:2048:+60M -t 1:ef00 "${DISK_IMG}" 2>/dev/null || true
    else
        log_warn "sgdisk non trovato, creando immagine senza GPT..."
    fi
    
    # Formatta la partizione come FAT32
    # Offset 1MB (2048 sectors * 512 bytes)
    local LOOP_OFFSET=$((2048 * 512))
    
    # Su macOS usiamo hdiutil, su Linux dd + mformat
    if [[ "$OSTYPE" == "darwin"* ]]; then
        # macOS: crea direttamente immagine FAT
        mformat -i "${DISK_IMG}@@${LOOP_OFFSET}" -F ::
        mmd -i "${DISK_IMG}@@${LOOP_OFFSET}" ::/EFI 2>/dev/null || true
        mmd -i "${DISK_IMG}@@${LOOP_OFFSET}" ::/EFI/BOOT 2>/dev/null || true
        mmd -i "${DISK_IMG}@@${LOOP_OFFSET}" ::/boot 2>/dev/null || true
        
        # Copia file
        if [ -f "${DEPS_DIR}/BOOTAA64.EFI" ]; then
            mcopy -i "${DISK_IMG}@@${LOOP_OFFSET}" "${DEPS_DIR}/BOOTAA64.EFI" ::/EFI/BOOT/ 2>/dev/null || true
        fi
        mcopy -i "${DISK_IMG}@@${LOOP_OFFSET}" "${BUILD_DIR}/kernel.bin" ::/boot/ 2>/dev/null || true
    fi
    
    log_info "Immagine disco creata: ${DISK_IMG}"
}

# ============================================================================
# ESEGUI CON QEMU
# ============================================================================
run_qemu() {
    log_info "Avviando QEMU..."
    
    # Compila se necessario
    if [ ! -f "${BUILD_DIR}/kernel.elf" ]; then
        log_info "Compilando..."
        cd "${PROJECT_DIR}"
        make all
    fi
    
    echo ""
    echo "========================================"
    echo "  QEMU AArch64 Virtual Machine"
    echo "========================================"
    echo ""
    echo "Per uscire: Ctrl+C (oppure Ctrl+A, poi X)"
    echo ""
    
    # Create dummy disk image for VirtIO block driver testing
    if [ ! -f "${BUILD_DIR}/disk.img" ]; then
        log_info "Creating dummy disk image (128MB)..."
        dd if=/dev/zero of="${BUILD_DIR}/disk.img" bs=1M count=128 2>/dev/null
    fi

    # Boot diretto del kernel ELF (standard per sviluppo QEMU)
    log_info "Boot kernel..."
    qemu-system-aarch64 \
        -M virt \
        -cpu cortex-a57 \
        -m 1G \
        -smp 4 \
        -kernel "${BUILD_DIR}/kernel.elf" \
        -serial mon:stdio \
        -display default,show-cursor=on \
        -device virtio-gpu-device \
        -device virtio-keyboard-device \
        -device virtio-mouse-device \
        -device virtio-blk-device,drive=hd0 \
        -drive file="${BUILD_DIR}/disk.img",format=raw,if=none,id=hd0
}

# ============================================================================
# ESEGUI CON DEBUG
# ============================================================================
run_debug() {
    log_info "Avviando QEMU in modalità debug..."
    
    echo ""
    echo "========================================"
    echo "  QEMU Debug Mode"
    echo "========================================"
    echo ""
    echo "QEMU in attesa su porta 1234"
    echo ""
    echo "In un altro terminale, esegui:"
    echo "  aarch64-none-elf-gdb ${BUILD_DIR}/kernel.elf"
    echo "  (gdb) target remote :1234"
    echo "  (gdb) layout asm"
    echo "  (gdb) stepi"
    echo ""
    
    qemu-system-aarch64 \
        -M virt \
        -cpu cortex-a57 \
        -m 1G \
        -smp 4 \
        -kernel "${BUILD_DIR}/kernel.elf" \
        -nographic \
        -s -S
}

# ============================================================================
# PULIZIA
# ============================================================================
clean() {
    log_info "Pulizia..."
    
    cd "${PROJECT_DIR}"
    make clean 2>/dev/null || true
    rm -rf "${BUILD_DIR}"
    rm -f boot/efi_stub.S
    
    log_info "Pulizia completata"
}

# ============================================================================
# MAIN
# ============================================================================
main() {
    echo ""
    echo "========================================"
    echo "  AArch64 OS Build System"
    echo "========================================"
    echo ""
    
    local cmd="${1:-all}"
    
    case "${cmd}" in
        clean)
            clean
            ;;
        deps)
            check_dependencies
            download_grub_efi
            ;;
        build)
            build_bootloader
            ;;
        iso)
            create_grub_config
            create_iso
            ;;
        disk)
            create_disk_image
            ;;
        run)
            run_qemu
            ;;
        debug)
            run_debug
            ;;
        all)
            check_dependencies
            download_grub_efi
            build_bootloader
            create_grub_config
            create_iso
            create_disk_image
            echo ""
            log_info "Build completato!"
            echo ""
            echo "File generati:"
            echo "  - ${BUILD_DIR}/kernel.elf"
            echo "  - ${BUILD_DIR}/kernel.bin"
            echo "  - ${BUILD_DIR}/efi.img"
            echo "  - ${BUILD_DIR}/aarch64-os.img"
            echo ""
            echo "Per eseguire:"
            echo "  ./build_iso.sh run      # Boot normale"
            echo "  ./build_iso.sh debug    # Boot con GDB"
            ;;
        *)
            echo "Uso: $0 [clean|deps|build|iso|disk|run|debug|all]"
            echo ""
            echo "Comandi:"
            echo "  clean  - Pulisce tutti i file generati"
            echo "  deps   - Installa dipendenze"
            echo "  build  - Compila bootloader e kernel"
            echo "  iso    - Crea immagine ISO"
            echo "  disk   - Crea immagine disco"
            echo "  run    - Esegue con QEMU"
            echo "  debug  - Esegue con QEMU in debug mode"
            echo "  all    - Esegue tutto (default)"
            exit 1
            ;;
    esac
}

main "$@"
