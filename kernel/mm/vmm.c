/*
 * kernel/mm/vmm.c
 * Virtual Memory Manager
 *
 * AArch64 4-level page table management
 */
#include <kernel/pmm.h>
#include <kernel/printk.h>
#include <kernel/string.h>
#include <kernel/vmm.h>

/* Page Table Levels */
#define PGD_SHIFT 39
#define PUD_SHIFT 30
#define PMD_SHIFT 21
#define PT_SHIFT 12

#define PGD_INDEX(x) (((x) >> PGD_SHIFT) & 0x1FF)
#define PUD_INDEX(x) (((x) >> PUD_SHIFT) & 0x1FF)
#define PMD_INDEX(x) (((x) >> PMD_SHIFT) & 0x1FF)
#define PT_INDEX(x) (((x) >> PT_SHIFT) & 0x1FF)

/* Global Kernel PGD */
static uint64_t *kernel_pgd;

/*
 * Get or create next level table
 */
static uint64_t *get_next_table(uint64_t *table, uint64_t index, int alloc) {
  if (table[index] & PTE_VALID) {
    /* Extract physical address mask (48 bits supported) */
    uint64_t phys = table[index] & 0x0000FFFFFFFFF000UL;
    /* In full OS, this should be mapped. For early boot, assuming
     * identity/linear map */
    return (uint64_t *)phys;
  }

  if (!alloc)
    return NULL;

  // Allocate new page for table
  void *page = pmm_alloc_page();
  if (!page)
    return NULL;

  uint64_t phys = (uint64_t)page;

  // Actually pmm_alloc_page returns physical address + offset if using early
  // mapping? Wait, pmm returns direct mapped address usually... Let's assume
  // PMM returns physical address for now as per previous impl? Checking pmm
  // implementation... PMM returns (MEMORY_BASE + pfn_to_phys) which is physical
  // address in QEMU space (0x4000...).

  // In our simplified model:
  // Physical address IS the pointer returned by pmm (since we are 1:1 mapped
  // currently) But table entries store PHYSICAL addresses.

  phys = (uint64_t)page; // This is actually physical in current setup

  table[index] = phys | PTE_TABLE | PTE_VALID | PTE_AF | PTE_INNER_SHARE |
                 PTE_RW | PTE_AP_EL1_RW | PTE_UXN | PTE_PXN;

  return page;
}

/*
 * Map a page
 */
int vmm_map_page(uint64_t *pgd, uint64_t virt, uint64_t phys, uint64_t flags) {
  uint64_t *pud, *pmd, *pt;

  /* Level 0: PGD */
  pud = get_next_table(pgd, PGD_INDEX(virt), 1);
  if (!pud)
    return -1;

  /* Level 1: PUD */
  pmd = get_next_table(pud, PUD_INDEX(virt), 1);
  if (!pmd)
    return -1;

  /* Level 2: PMD */
  pt = get_next_table(pmd, PMD_INDEX(virt), 1);
  if (!pt)
    return -1;

  /* Level 3: PT */
  pt[PT_INDEX(virt)] = phys | flags;

  return 0;
}

/*
 * Unmap a page
 */
void vmm_unmap_page(uint64_t *pgd, uint64_t virt) {
  uint64_t *pud, *pmd, *pt;

  pud = get_next_table(pgd, PGD_INDEX(virt), 0);
  if (!pud)
    return;

  pmd = get_next_table(pud, PUD_INDEX(virt), 0);
  if (!pmd)
    return;

  pt = get_next_table(pmd, PMD_INDEX(virt), 0);
  if (!pt)
    return;

  pt[PT_INDEX(virt)] = 0;

  /* TODO: TLB invalidate */
  __asm__ volatile("tlbi vaae1is, %0" ::"r"(virt >> 12));
  __asm__ volatile("dsb ish");
  __asm__ volatile("isb");
}

/*
 * Initialize VMM and Enable MMU
 */
void vmm_init(void) {
  pr_info("VMM: Initializing MMU...\n");

  /* Allocate Kernel PGD */
  kernel_pgd = pmm_alloc_page();
  if (!kernel_pgd) {
    pr_err("VMM: Failed to allocate kernel PGD\n");
    return;
  }
  memset(kernel_pgd, 0, 4096);

  /* 1. Identity Map RAM (1GB from 0x40000000) */
  /* We use 2MB blocks or 4KB pages. For simplicity, let's use 4KB pages for now
   */
  uint64_t ram_start = 0x40000000UL;
  uint64_t ram_size = 1024UL * 1024 * 1024; /* 1GB */
  for (uint64_t addr = ram_start; addr < ram_start + ram_size; addr += 4096) {
    vmm_map_page(kernel_pgd, addr, addr, PAGE_KERNEL);
  }

  /* 2. Identity Map MMIO (UART, GIC, VirtIO) */
  /* 0x08000000 to 0x0A000000 covers typical QEMU virt devices */
  for (uint64_t addr = 0x08000000UL; addr < 0x0A800000UL; addr += 4096) {
    vmm_map_page(kernel_pgd, addr, addr, PAGE_DEVICE);
  }

  /* 3. Setup MAIR_EL1 (Memory Attribute Indirection Register) */
  /* Index 0: Normal Memory, Inner/Outer Write-Back Non-transient,
   * Read-Allocate, Write-Allocate */
  /* Index 1: Device Memory, nGnRE (non-Gathering, non-Reordering, Early Write
   * Acknowledgement) */
  uint64_t mair = (0xFFUL << 0) | /* Index 0: Normal */
                  (0x04UL << 8);  /* Index 1: Device nGnRE */
  __asm__ __volatile__("msr mair_el1, %0" : : "r"(mair));

  /* 4. Setup TCR_EL1 (Translation Control Register) */
  /* TG0=0 (4KB), SH0=3 (Inner Shareable), ORGN0=1 (WB/WA), IRGN0=1 (WB/WA) */
  /* T0SZ=16 (48-bit VA, 64 - 48 = 16) */
  /* IPS=0 (32-bit PA) or 2 (40-bit PA). QEMU virt uses 40-bit or 32-bit? */
  /* Let's use IPS=2 (40-bit) to be safe */
  uint64_t tcr = (16UL << 0) | /* T0SZ */
                 (3UL << 12) | /* SH0 Inner Shareable */
                 (1UL << 10) | /* ORGN0 WB/WA */
                 (1UL << 8) |  /* IRGN0 WB/WA */
                 (2UL << 32);  /* IPS 40-bit */
  __asm__ __volatile__("msr tcr_el1, %0" : : "r"(tcr));

  /* 5. Set TTBR0_EL1 */
  __asm__ __volatile__("msr ttbr0_el1, %0" : : "r"((uint64_t)kernel_pgd));

  /* 6. Enable MMU in SCTLR_EL1 */
  uint64_t sctlr;
  __asm__ __volatile__("mrs %0, sctlr_el1" : "=r"(sctlr));
  sctlr |= (1UL << 0) |  /* M: MMU enable */
           (1UL << 12) | /* I: Instruction cache enable */
           (1UL << 2);   /* C: Data cache enable */
  __asm__ __volatile__("msr sctlr_el1, %0" : : "r"(sctlr));
  __asm__ __volatile__("isb");

  pr_info("VMM: MMU Enabled. Kernel PGD at %p\n", kernel_pgd);
}

/*
 * Create a new PGD
 */
/*
 * Create a new PGD
 */
uint64_t *vmm_create_pgd(void) {
  uint64_t *pgd = (uint64_t *)pmm_alloc_page();
  if (!pgd)
    return NULL;

  /* Copy kernel mappings from current TTBR0 */
  /* This ensures the kernel remains mapped when we switch to this PGD */
  uint64_t current_ttbr0;
  __asm__ __volatile__("mrs %0, ttbr0_el1" : "=r"(current_ttbr0));

  /* Mask out ASID (top 16 bits often reserved/used for ASID) if any */
  /* Assuming physical address is valid pointer (identity map) */
  uint64_t *src_pgd = (uint64_t *)(current_ttbr0 & 0x0000FFFFFFFFF000UL);

  memcpy(pgd, src_pgd, 4096);

  return pgd;
}

/*
 * Destroy a PGD
 */
void vmm_destroy_pgd(uint64_t *pgd) {
  if (pgd)
    pmm_free_page((void *)pgd);
}
