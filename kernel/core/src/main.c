/*
 * kernel/main.c
 * OS1 Microkernel Entry Point
 */

#include <libkernel/types.h>
#include <core/printk.h>
#include <core/cpu.h>
#include <core/hal.h>
#include <core/pmm.h>
#include <core/vmm.h>
#include <core/sched.h>
#include <core/irq.h>
#include <core/fdt.h>
#include <core/boot_fs.h>
#include <core/registry.h>
#include <core/drivers.h>

#define OS1_VERSION_STR "0.2.0-micro"

#ifdef ARCH_AMD64
void kernel_main(uint64_t magic, uint64_t mbi_ptr);
#else
void kernel_main(uint64_t fdt_ptr);
#endif

static void print_banner(void) {
    printk("\n========================================\n");
    printk("   OS1 Microkernel v%s\n", OS1_VERSION_STR);
    printk("   Architecture: %s\n",
#ifdef ARCH_AMD64
        "AMD64"
#else
        "AArch64"
#endif
    );
    printk("========================================\n\n");
}

/* Phase 2: Register all active drivers in the hierarchical registry.
 * Drivers register under sys/drivers/<name>/ with status, type, and
 * an optional IPC queue path for future message-based commands. */
static void register_drivers(void) {
#ifdef ARCH_AMD64
    registry_set("sys/drivers/uart/type",    "16550");
    registry_set("sys/drivers/uart/status",  "active");
    reg_ipc_init_queue("sys/drivers/uart/cmd");
    uart_16550_driver_register();

    registry_set("sys/drivers/pic/type",     "PIC-PIT");
    registry_set("sys/drivers/pic/status",   "active");

    registry_set("sys/drivers/pci/type",     "PCI-bus");
    registry_set("sys/drivers/pci/status",   "active");
    reg_ipc_init_queue("sys/drivers/pci/cmd");
#else
    registry_set("sys/drivers/uart/type",    "PL011");
    registry_set("sys/drivers/uart/status",  "active");
    reg_ipc_init_queue("sys/drivers/uart/cmd");
    pl011_driver_register();

    registry_set("sys/drivers/gic/type",     "GICv2");
    registry_set("sys/drivers/gic/status",   "active");

    registry_set("sys/drivers/timer/type",   "ARM-Generic");
    registry_set("sys/drivers/timer/status", "active");
#endif

    registry_set("sys/drivers/virtio-blk/type",   "VirtIO-BLK");
    registry_set("sys/drivers/virtio-blk/status", "active");
    reg_ipc_init_queue("sys/drivers/virtio-blk/cmd");
    virtio_blk_driver_register();

    registry_set("sys/drivers/virtio-gpu/type",   "VirtIO-GPU");
    registry_set("sys/drivers/virtio-gpu/status", "active");
    reg_ipc_init_queue("sys/drivers/virtio-gpu/cmd");

    registry_set("sys/drivers/keyboard/type",   "VirtIO-Input");
    registry_set("sys/drivers/keyboard/status", "active");

    pr_info("Registry: Driver registration complete\n");
}

#ifdef ARCH_AMD64
void kernel_main(uint64_t magic, uint64_t mbi_ptr) {
    (void)magic; (void)mbi_ptr;
    /* TODO: extract boot info from Multiboot2 tags */
#else
void kernel_main(uint64_t fdt_ptr) {
    fdt_init(fdt_ptr);
#endif

    /* 1. Early console */
    extern void driver_console_init(void);
    driver_console_init();

    /* 2. CPU vectors + per-CPU setup */
    cpu_init();

    /* 1.5 Early platform init */
    extern void arch_platform_early_init(void);
    arch_platform_early_init();

    print_banner();

    /* 3. Memory management bootstrap */
    size_t count = 0;
    extern struct mem_region *arch_platform_get_mem_regions(size_t *count);
    struct mem_region *regions = arch_platform_get_mem_regions(&count);
    pmm_init(regions, count);
    vmm_init();
    vmm_dynamic_remap();

    /* 4. IRQ + Timer */
    irq_init();
    extern void irq_init_percpu(void);
    irq_init_percpu();
    extern void driver_timer_init(void);
    driver_timer_init();
    extern void timer_init_percpu(void);
    timer_init_percpu();

    /* 5. Scheduler */
    process_init();
    extern void smp_create_idle_task(uint32_t cpu_id);
    smp_create_idle_task(0);

    /* 6. Registry (Phase 3 — hierarchical registry with IPC queues) */
    registry_init();

    /* 7. Hardware bus + block device */
    extern void hal_bus_init(void);
    hal_bus_init();

    extern void virtio_blk_init(void);
    virtio_blk_init();

    /* 8. Boot filesystem (Ext4 on VirtIO-blk) */
    if (boot_fs_init() != 0) {
        panic("Microkernel: Failed to initialize boot filesystem");
    }

    /* 8b. VFS layer (Phase 3a) — mount Ext4 as root filesystem */
    extern void vfs_init(void);
    vfs_init();

    /* 9. Phase 2 — register drivers in hierarchical registry */
    register_drivers();

    /* 10. Spawn Init (PID 1) — it will launch VFS + compositor daemons */
    struct process *init_proc = process_create("init", PROC_PRIO_USER,
                                               PROC_PERM_SYSTEM);
    if (init_proc && process_load_elf(init_proc, "/sys/bin/init") == 0) {
        pr_info("Microkernel: Spawning Init (PID %d)\n", init_proc->pid);
        enqueue_task(init_proc);
    } else {
        panic("Microkernel: Failed to load /sys/bin/init");
    }

    /* 11. Enter scheduler loop */
    pr_info("%s", "Microkernel: Initialization complete. Entering supervisor loop.\n");
    local_irq_enable();

    while (1) {
        hal_cpu_idle();
    }
}
