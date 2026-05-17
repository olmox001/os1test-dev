# Plan Fix: OS1 Microkernel — Critique and Corrections

This document critiques the current refactoring plan (REFACTOR_PLAN.md / STATUS.md) against the
actual codebase state, organized by severity. It does not propose new phases — it corrects what the
existing plan gets wrong so Phase 3 and 4 can proceed without building on false assumptions.

---

## Tier A — False Completion Claims

These items are marked complete in STATUS.md or WORK_SUMMARY.md but are contradicted by the code.

### A1. `ipc.h` Namespace Leakage Is NOT Fixed

**Claim**: Phase 1/2 "COMPLETED — Namespace Sanitization: files in `kernel/` never import headers
from outside `kernel/`."

**Reality**: The Makefile includes `user/sys/include` on the kernel's compiler path:

```
# Makefile line 84
INCLUDE = -Ikernel/core/include ... -Iuser/sys/include
```

`kernel/core/include/core/ipc.h` contains only `#include <ipc.h>`, which resolves to
`user/sys/include/ipc.h`. The `reg_msg` and `ipc_message` structs used throughout kernel-space
(`registry.c`, `syscall.c`, `drivers.h`) are defined in a **userland header**. This is the exact
same cross-boundary leakage that was fixed for `elf.h`, and it was not applied to `ipc.h`.

**Fix required**:
- Move `struct reg_msg` and `struct ipc_message` into a dedicated
  `kernel/libkernel/include/libkernel/ipc_types.h` (no userland includes, uses only
  `libkernel/types.h`).
- Have `user/sys/include/ipc.h` include that file from a shared location, or redefine the
  user-facing message types independently (they are ABI, not implementation).
- Remove `user/sys/include` from kernel CFLAGS and add `-Ikernel/libkernel/include` where needed.

---

### A2. Registry "Zero-Heap-Loss" Claim Is False

**Claim** (STATUS.md, Best Practices #1): "The registry operates via static node pools guaranteeing
absolute absence of heap fragmentation."

**Reality**: `reg_ipc_init_queue()` in `registry.c:212` calls `kmalloc(sizeof(struct reg_queue))`.
Each driver queue (`sys/drivers/uart/cmd`, `sys/drivers/pci/cmd`, etc.) heap-allocates a 386-byte
`reg_queue` at registration time. The *nodes* are static, but the *queues* are heap. With 5+ driver
queues registered in `main.c:register_drivers()`, this is not zero-heap-loss.

**Fix required**: Either pre-allocate a static pool of `reg_queue` objects (parallel to
`node_pool[]`) or update documentation to accurately state: "nodes are static-pooled; IPC queues
use bounded early-boot heap allocation."

---

### A3. `REG_POOL_SIZE` Documented as 128, Actually 256

**Claim** (STATUS.md): "Static pool: `REG_POOL_SIZE = 128`."

**Reality**: `kernel/core/include/core/registry.h:9` defines `#define REG_POOL_SIZE 256`.

**Fix required**: Update STATUS.md to match the header. No code change needed.

---

### A4. Phase 4 Step 1 Is Already Complete

**Claim** (REFACTOR_PLAN.md Phase 4): "Audit userland includes… ensure `user/sys/include/elf.h`
is strictly decoupled from `kernel/core/include/core/elf.h`."

**Reality**: WORK_SUMMARY.md records this as done ("Rewrote the kernel's dedicated
`kernel/core/include/core/elf.h` to be 100% self-contained"). Both headers exist and are
independent. This step is complete.

**Fix required**: Mark Phase 4 Step 1 as `[x] DONE` in REFACTOR_PLAN.md. The remaining Phase 4
work is Step 2 (namespace audit for `ipc.h` — which is now Tier A1 above).

---

### A5. Compositor Is Still Kernel-Resident

**Claim** (architectural philosophy): OS1 is described as a microkernel with isolated servers
communicating via IPC.

**Reality**: `syscall.c` dispatches 10 compositor syscalls (`SYS_CREATE_WINDOW`,
`SYS_WINDOW_BLIT`, `SYS_COMPOSITOR_RENDER`, etc.) that call compositor functions living in
`kernel/core/src/graphics/compositor.c`. The comment in `syscall.c:143` says "migrating to
user-space daemon" but there is no phase entry, no timeline, and no partial migration. The graphics
stack is a fully in-kernel monolith.

**Fix required**: Either add a Phase 3.5 entry to REFACTOR_PLAN.md that explicitly plans the
compositor migration steps, or acknowledge in STATUS.md that the kernel-resident compositor is a
known deliberate exception to the microkernel model (performance tradeoff). The current state of
claiming microkernel design while shipping a kernel-resident window compositor creates a false
picture.

---

## Tier B — Phase 3 Design Defects

These are problems with the Phase 3 plan itself, not its completion status.

### B1. VFS Prerequisite Does Not Exist

**The plan** (REFACTOR_PLAN.md Phase 3, step 3): "Mount `/sys/registry` into the virtual
filesystem tree so that subsystems write and read hardware attributes using standard file
descriptors."

**The reality**: `sys_open/read/write/close` in `stubs.c` all return `-ENOSYS`. `vfs.h` contains
exactly one function prototype (`vfs_resolve_path`). There is no:
- File descriptor table (per-process)
- Vnode layer
- Mount table
- VFS operation dispatch (`vfsops`)

You cannot "mount" into a VFS that does not exist. Phase 3 step 3 depends on infrastructure that
must be built first.

**Fix required**: Split Phase 3 into two sequential sub-phases:

- **Phase 3a — VFS Skeleton** (prerequisite):
  1. Per-process fd table (array of `struct vnode *` or `struct file *`, fd 0/1/2 pre-wired).
  2. `struct vfsops` and `struct vnodeops` interface stubs in `vfs.h`.
  3. Mount table (`vfs_mount_table[]`) with a single initial entry for the boot Ext4.
  4. Implement `sys_open/read/write/close` routing through the vnode layer.

- **Phase 3b — Registry-VFS Bridge** (the actual Phase 3 goal):
  1. Implement `reg_vfs_open/read/write/readdir` that translate fd operations into
     `registry_get/registry_set/registry_list` calls.
  2. Register the registry as a pseudo-filesystem type in the mount table at `/sys/registry`.
  3. Test with the example from the plan: `open("/sys/registry/sys/drivers/uart/type", O_RDONLY)`.

---

### B2. Path Namespace Mismatch

**The plan's example**:
```c
int fd = open("/sys/registry/hardware/uart/baud_rate", O_WRONLY);
```

**The actual registry paths** (set in `main.c:register_drivers()`):
```c
registry_set("sys/drivers/uart/type", "PL011");
```

Two discrepancies:
1. The plan uses `/hardware/uart/` but the code uses `sys/drivers/uart/` — the `hardware/`
   subtree does not exist anywhere.
2. Registry paths use no leading slash (`sys/drivers/...`) but `reg_lookup` strips a leading slash
   if present, so both would work. However the VFS mount path `/sys/registry/` plus registry path
   `sys/drivers/uart/type` would produce `/sys/registry/sys/drivers/uart/type`, not the blueprint's
   `/sys/registry/hardware/uart/baud_rate`.

**Fix required**: Before writing Phase 3b code, decide on a canonical path layout and update
REFACTOR_PLAN.md to reflect it. Suggested: keep `sys/drivers/<name>/` for driver metadata,
add `sys/hardware/<name>/` for FDT-discovered raw hardware resources (base_addr, irq, etc.).
Update the Phase 3 example to use real paths.

---

### B3. AMD64 Autodiscovery Is Blocked by a TODO

**The plan** (Phase 3 step 2): "Parse FDT (AArch64) and Multiboot v2 tags (AMD64) at boot,
populating `/sys/registry/hardware/` dynamically."

**The reality**: `kernel/core/src/main.c:87`:
```c
void kernel_main(uint64_t magic, uint64_t mbi_ptr) {
    (void)magic; (void)mbi_ptr;
    /* TODO: extract boot info from Multiboot2 tags */
```

The Multiboot2 pointer is discarded. AMD64 hardware autodiscovery cannot populate IRQs or MMIO
addresses without parsing this structure. AArch64 FDT parsing exists (`fdt.c`) but is not called
from `register_drivers()` — only from `kernel_main()` before the registry is initialized.

**Fix required**: Phase 3b autodiscovery must explicitly list as a prerequisite:
- AMD64: implement Multiboot2 tag walker and extract memory map + ACPI/RSDP pointer.
- Both arches: call FDT/Multiboot2 parsing *after* `registry_init()` and populate
  `sys/hardware/<device>/base_address` and `sys/hardware/<device>/irq` from the parsed data.

---

### B4. Phase 3 Execution Order Is Wrong

Current blueprint order:
1. Registry Hierarchy Setup
2. Hardware Autodiscovery
3. VFS Mounting

Correct order given the above defects:

1. **Fix ipc.h leakage** (Tier A1) — required before any further kernel/userland boundary work.
2. **Decide path namespace** (B2) — documents before code.
3. **VFS Skeleton** (B1 / Phase 3a) — fd table, vnode, mount table.
4. **Registry-VFS Bridge** (B1 / Phase 3b) — mount registry into VFS.
5. **Hardware Autodiscovery** (B3) — AMD64 Multiboot2 parser + FDT post-registry-init call.

Steps 4 and 5 can proceed in parallel once Step 3 is done.

---

## Tier C — Documentation Drift (Minor Fixes)

| Location | Error | Fix |
|:---|:---|:---|
| STATUS.md, Best Practices #1 | `REG_POOL_SIZE = 128` | Change to 256 |
| STATUS.md, Phase 3 | Listed as `Pianificato / Disegnato` | Split into 3a (VFS Skeleton) and 3b (Registry Mount) |
| REFACTOR_PLAN.md, Phase 4 Step 1 | Listed as action item | Mark `[x] DONE` |
| MANIFEST.md, User Space section | `shell.c` path references wrong repo root | Verify actual path |
| REFACTOR_PLAN.md, Phase 3 example | `/sys/registry/hardware/uart/baud_rate` | Update to match actual registry path layout once decided |
| STATUS.md | Claims `REG_QUEUE_DEPTH = 16` per node — correct, but queue allocation from heap is undocumented | Add note: "queues are heap-allocated at driver registration time" |

---

## Summary: What Must Be Done Before Phase 3 Code Starts

1. **Fix `ipc.h`**: Move `struct reg_msg` / `struct ipc_message` out of `user/sys/include/ipc.h`
   into a kernel-private header; remove `user/sys/include` from kernel CFLAGS.
2. **Decide canonical path layout** for registry entries and update REFACTOR_PLAN.md Phase 3
   example to match.
3. **Add Phase 3a** (VFS Skeleton) explicitly to REFACTOR_PLAN.md before Phase 3b.
4. **Add Multiboot2 parser stub** to AMD64 `kernel_main()` so hardware autodiscovery is unblocked.
5. **Update STATUS.md** to fix the three factual errors (pool size, heap-loss claim, Phase 4 Step 1
   status) so it remains a reliable truth source.
