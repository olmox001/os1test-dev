# FASE 1: Consolidation & Refactoring Plan

**Audience**: Implementation guide for FASE 2-8  
**Status**: ✅ READY FOR IMPLEMENTATION

---

## PHASE 1 OUTCOMES & IMMEDIATE ACTIONS

### What FASE 1 Achieved
- ✅ Identified 30% boot code duplication (low consolidation ROI)
- ✅ Catalogued 7 hardcoded MMIO addresses
- ✅ Established baseline metrics: 21.6% assembly ratio
- ✅ Confirmed both architectures boot stably
- ✅ Identified root causes: device discovery + HAL incompleteness

### What FASE 1 Did NOT Change
- No code modifications (audit-only phase)
- No new build artifacts
- No functional changes

---

## IMMEDIATE OPPORTUNITIES (Quick Wins)

These can be done immediately in FASE 2 without risk:

### 1. Create Unified Platform Header
**File to create**: `kernel/include/kernel/platform_def.h`  
**Purpose**: Single source of truth for platform constants  
**Content**:
```c
#ifndef _KERNEL_PLATFORM_DEF_H
#define _KERNEL_PLATFORM_DEF_H

/* Platform device addresses — arch-specific includes handle this */
#ifdef ARCH_AARCH64
  #include "platform_aarch64.h"
#elif ARCH_AMD64
  #include "platform_amd64.h"
#endif

/* Common platform functions */
struct device_enum {
    uint64_t base_addr;
    uint32_t irq;
    char name[32];
};

/* To be implemented per-arch */
int platform_enumerate_devices(struct device_enum *out, size_t max);

#endif
```

**Files to create**:
- `kernel/arch/aarch64/include/platform_aarch64.h` — Move current platform.h content here
- `kernel/arch/amd64/include/platform_amd64.h` — amd64 platform constants

**Effort**: 1-2 hours (refactoring, no new logic)  
**Benefit**: Prepares for HAL unification (FASE 2)  
**Risk**: VERY LOW (pure reorganization)

---

### 2. Extract Multiboot2 Parser to C
**File to create**: `kernel/boot/multiboot2_parser.c`  
**Purpose**: Shared parser logic for both architectures  
**Current duplication**: ~30 lines assembly per arch  
**New structure**:
```c
// kernel/boot/multiboot2_parser.c
struct mb2_info {
    uint32_t total_size;
    uint32_t reserved;
    // ... tags parsed
    uint64_t kernel_entry;
    void *mmap_tag;
};

int mb2_parse_info(uint64_t info_ptr, struct mb2_info *out) {
    // Shared logic
    // Fills out->kernel_entry, out->mmap_tag, etc.
}
```

**Files to modify**:
- `boot/aarch64/stage1.S` — Replace parse_multiboot2_info() call with C function call
- `boot/amd64/stage1.S` — Same

**Effort**: 2-3 hours  
**Benefit**: -30 lines assembly, improves maintainability  
**Risk**: LOW (parser logic same, just reorganized)

---

### 3. Document Platform-Specific Device Bases
**File to create**: `kernel/arch/*/platform_devices.c`  
**Purpose**: Centralize device discovery stubs for each arch  
**aarch64 content**:
```c
// kernel/arch/aarch64/platform_devices.c
#include <kernel/platform_def.h>

int platform_enumerate_devices(struct device_enum *out, size_t max) {
    int count = 0;
    
    // UART
    if (count < max) {
        out[count].base_addr = PLATFORM_UART_BASE;
        out[count].irq = PLATFORM_IRQ_UART0;
        strcpy(out[count].name, "pl011");
        count++;
    }
    
    // GIC
    if (count < max) {
        out[count].base_addr = PLATFORM_GICD_BASE;
        out[count].irq = 0;  // not an IRQ target
        strcpy(out[count].name, "gic-v2");
        count++;
    }
    
    // ... more devices
    return count;
}
```

**amd64 content**:
```c
// kernel/arch/amd64/platform_devices.c
int platform_enumerate_devices(struct device_enum *out, size_t max) {
    int count = 0;
    
    // Enumerate via PCI (to be implemented)
    // UART via 16550 at I/O port 0x3F8
    // APIC at 0xFEE00000
    // ... etc
    
    return count;
}
```

**Effort**: 1 hour  
**Benefit**: Prepares device abstraction for FASE 3  
**Risk**: MINIMAL (stub implementations)

---

## PHASE 2 DEPENDENCY CHAIN

For FASE 2 (HAL Layer 1), these must be ready first:

```
FASE 1 Outputs
  ├─ platform_def.h (unified header)
  ├─ platform_aarch64.h (aarch64 constants)
  ├─ platform_amd64.h (amd64 constants)
  └─ multiboot2_parser.c (shared boot)
       ↓
  FASE 2: HAL Layer 1
     └─ Unify arch-specific HAL functions
        (CPU ops, MMU, IRQ primitives)
```

---

## DETAILED REFACTORING ROADMAP

### Phase 2: HAL Layer 1 (Unification of Primitives)

**Target**: Unify CPU/memory/IRQ primitives behind clean HAL  

**Files to modify/create**:
1. `kernel/arch/*/cpu/hal_cpu.c` — Extract CPU init to functions
2. `kernel/arch/*/mm/hal_mmu.c` — Extract paging setup to functions  
3. `kernel/arch/*/irq/hal_irq.c` — Create unified IRQ handler registration

**Estimated effort**: 4-5 hours  
**Unblocks**: FASE 3 (driver abstraction)

---

### Phase 3: Driver MMIO/PCI Abstraction

**Target**: Devices discoverable via platform enumeration  

**Key changes**:
1. Replace hardcoded `0x09000000` with `PLATFORM_UART_BASE` lookup
2. Create driver registration: `driver_register(dev_enum, driver_ops)`
3. Move VirtIO enumeration from MMIO to PCI (amd64)

**Estimated effort**: 5-6 hours  
**Unblocks**: Phase 4, full amd64 support

---

### Phase 4: Device Tree Loader

**Target**: Dynamic device discovery for amd64  

**Key changes**:
1. Parse DTB passed by bootloader
2. Populate device_enum array from DTB
3. Fallback to hardcoded for QEMU w/o DTB

**Estimated effort**: 4-5 hours

---

## CONSOLIDATION OPPORTUNITIES (DEFERRED)

These have LOW ROI and are deferred:

### Boot Assembly Consolidation (Not recommended)
**Why deferred**: Paging logic is architecture-specific, savings minimal (~30 lines).  
**Decision**: Keep boot/ files separate. Consolidation not worth maintenance cost.

---

## TESTING STRATEGY FOR SUBSEQUENT PHASES

**After each FASE**:
1. Baseline compile: `make clean && make -B ARCH=aarch64`
2. Boot test: `timeout 30 make run ARCH=aarch64`
3. Verify: 3/3 unit tests pass, no panics
4. Repeat for amd64

**Extended stability** (after phase completion):
5. Extended run: `timeout 120 make run ARCH=aarch64` (twice, no regressions)
6. Check metrics: Assembly lines, code duplication, warnings

---

## RISK MITIGATION

### Low Risk Operations (Go ahead)
- Reorganizing files (platform_def.h)
- Creating new C files with stubs
- Adding new HAL functions

### Medium Risk Operations (Proceed carefully)
- Extracting assembly to C (mb2_parser.c)
- Modifying boot flow (test aarch64 first!)

### High Risk Operations (Test heavily)
- Modifying paging/MMU code
- Changing interrupt flow

---

## SUCCESS CRITERIA FOR FASE 1

✅ **Completed**:
- Audit report finished
- Consolidation plan documented
- Both architectures stable (baseline)
- Metrics established

**Sign-off**: Ready for FASE 2 implementation

---

## QUICK REFERENCE: File Changes by Phase

```
FASE 2: Platform Abstraction
  Create: kernel/include/kernel/platform_def.h
  Create: kernel/arch/aarch64/include/platform_aarch64.h
  Create: kernel/arch/amd64/include/platform_amd64.h
  Create: kernel/arch/*/platform_devices.c

FASE 3: Device Discovery  
  Modify: kernel/drivers/*/
  Create: kernel/hal/device_discovery.c

FASE 4: Device Tree
  Create: kernel/boot/device_tree.c
  Modify: kernel/main.c

FASE 5-8: As per ARCHITECTURE.md
```

---

## IMMEDIATE NEXT STEP

**PROCEED TO FASE 2**: Implement platform abstraction header + parser extraction  
**Estimated duration**: 4-6 hours  
**Go/No-Go decision**: ✅ GO (low risk, high value)
