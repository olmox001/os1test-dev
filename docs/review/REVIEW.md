# OS1 / NEXS — Master Code Review

> Comprehensive, evidence-based review of the whole codebase (368 source files,
> ~107k LOC). Classification per [`TAXONOMY.md`](TAXONOMY.md): severity **W0–W5** ×
> **kind**. Per-subsystem detail under [`analysis/`](analysis/). Companion:
> [`../PROJECT_CHARTER.md`](../PROJECT_CHARTER.md) (purpose & target architecture).
>
> Review date: 2026-06-02 · Branch: `comprehensive-review` · Build: **[verified]** both arches.
> All 9 subsystems analysed (fs included). Agent-delegated docs are maintainer spot-checked.

---

## 1. Headline: verified runtime behaviour (claim vs reality)

The project is presented as "boots correctly on `make run` for both arches, graphical,
virtio, detecting CPUs, mapping up to 4GB, running GUI + TTY shell." Built and run
headless (QEMU, serial capture):

| Capability | aarch64 (`make run`) | amd64 (`make run`, `-kernel`) |
|---|---|---|
| Builds (strict `-Werror`…) | ✅ clean | ✅ clean |
| Boots to **TTY shell in a composited window** | ✅ | ✅ **at `-m 3G`** |
| RAM detection | ✅ **3967 MB, dynamic** | ❌ **magic `0x0` → hardcoded 1 GB** (ignores `-m`) |
| Maps "up to 4GB" | ✅ | ❌ (1 GB only) |
| SMP (cores online) | ✅ **4/4** (work-stealing) | ⚠️ boots; weak detection |
| `-m 4G` | ✅ | ❌ **crash** (virtio queue size 0 → divide-by-zero) |

**Verdict:** the aarch64 path genuinely delivers the stated asset. The amd64 path
delivers it **only at ≤~3 GB via `make run`**; the 4GB/real-memory-map path works only
through the **GRUB-ISO (`make release`)** route — which is what the old, more confident
reports actually tested. This is the single most important correction in the review.

### The amd64 critical chain (root-caused, verified)
`make run` boots amd64 via QEMU `-kernel` → PVH entry (kernel has MB2+PVH headers, **no
MB1**) → magic arrives as `0x0` (**BOOT-01**) → platform hardcodes **1 GB** (**BOOT-02**) →
at `-m 4G`, QEMU puts the virtio-pci 64-bit BAR above 4 GB → **`pci_get_bar` truncates it
to 32 bits** (**DRV-VIRTIO-01, W5**) → `QUEUE_NUM_MAX` reads 0 → divide-by-zero → amd64
has **no user/kernel fault isolation** so it **halts** (**EXC-AMD64-02**).

---

## 2. Severity rollup (all 9 subsystems)

| Severity | Count | Meaning |
|---|---|---|
| **W5** Critical | 3 | DRV-VIRTIO-01 (4G crash), EXT4-01 (extent format), VFS-01 (no VFS layer) |
| **W4** Severe | 9 | boot/4GB, stack-DMA, IRQ no-op, font UAF, ELF map-escape, no-capabilities, ext4 write+no-ACL |
| **W3** Significant | 60 | bugs/SMP-races/security/wrong-design on used paths |
| **W2** Moderate | 101 | limitations, partial behaviour, refinements |
| **W1** Minor | 42 | dead code, stale comments, micro-perf |
| **W0** Info | 5 | cosmetic |
| **Total** | **~220** | actionable tier (W3+) = **72** |

## 3. Cross-cutting themes → foundations (the refactor spine)

These recur across subsystems and are the dependency-ordered foundations (see charter §5):

1. **PA/VA model + W^X** — everything silently assumes identity mapping; all RAM is mapped
   executable. (MM-VMM-01/02, AMMU-01/02, ELF-02)
2. **Coherent, capability-checked ABI** — mixed/duplicated syscall numbers, no errno, no fd
   table, **zero capability checks**. (ABI-01/02/03/04, USR-SEC-01/02/03, LIB-REG-02)
3. **Real allocators** — kmalloc never frees to PMM; PMM has no buddy; userland malloc gaps.
   (MM-KM-01, MM-PMM-02/03, USR-MALLOC-01..05)
4. **Boot stability (GPLv2-compatible loader)** — amd64 boot-protocol mishandling. (BOOT-01/02)
5. **Thin HAL + real drivers/device-tree** — over-abstracted MMIO path; many driver stubs/races.
   (HAL-01, DRV-*, ARCH-01/02)
6. **Service isolation (seL4)** — compositor/font/vfs in-kernel and entangled with the scheduler.
   (SCHED-01, GFX-COMP-03, GFX-FONT-01, USR-SEC-03)
7. **SMP correctness** — no TLB shootdown; multiple lock-free shared-state races.
   (MM-VMM-05, AMMU-08, IRQ-02, DRV-UART-01, GFX-COMP-01/02, UACC-*-TOCTOU)
8. **Process/IPC model** — lost-wakeup race, unbounded IPC, non-blocking wait, no W^X for users.
   (IPC-01, SCHED-05, ELF-01)

---

## 4. Critical & severe findings (W5 / W4) — full

| ID | Sev | Kind | Location | Summary |
|----|-----|------|----------|---------|
| DRV-VIRTIO-01 | W5 | BUG | `pci/pci.c:106`, `amd64/hal.c:26-37` | 64-bit BAR truncated to 32 bits → garbage MMIO base at `-m 4G` → queue size 0 → divide-by-zero. **Root cause of the amd64 4G crash.** |
| BOOT-01 | W4 | BUG·WRONG-DESIGN | `amd64/boot/start.S`; `platform.c:157-186` | No MB1 header; QEMU `-kernel` uses PVH; magic expected in a register never matches → unknown protocol. |
| BOOT-02 | W4 | BUG | `platform.c:173-185` | Falls back to hardcoded 1 GB; 4GB unreachable on `make run`; fragile → 4G crash. |
| DRV-VIRTIO-03 | W4 | BUG·SECURITY | `virtio/virtio_blk.c:99-120` | `req`/`status` on the kernel **stack** used as DMA targets; device writes to stack; no coherency/alignment. |
| IRQ-01 | W4 | WRONG-DESIGN | `timer/pic_pit.c:57-59`, `irq/irq.c:87-135` | `acknowledge()` always returns 1023; generic `irq_handler` is a no-op on amd64; real dispatch bypasses the chip EOI contract. |
| GFX-FONT-01 | W4 | SECURITY·BUG | `graphics/font.c:174-191`, `syscall_dispatch.c:234` | `sys_set_font` stores a **raw user pointer** into kernel globals, dereferenced during IRQ-context rendering → UAF / info-leak. |
| ELF-01 | W4 | SECURITY | `sched/elf.c:48-92` | No `p_vaddr` range check; process PGDs share kernel upper-half by reference → crafted ELF can corrupt kernel page tables. |
| ABI-04 | W4 | SECURITY | `core/syscall_dispatch.c:151,166,176,251` | No capability checks: any process can kill any PID, steal focus, destroy windows, write any file. |
| EXT4-01 | W5 | BUG·MISSING | `fs/ext4.c:278-316`, `ext4.h:103` | Driver never reads `i_flags`; can't detect ext4 **extent-format** inodes → garbage reads on standard `mkfs.ext4` images (works only via `mkdisk`'s hand-built block-mapped inodes). |
| VFS-01 | W5 | WRONG-DESIGN | `fs/vfs.c`, `vfs.h` | "VFS" is a 59-line path-normaliser: no vnode, no mount table, no `file_ops`; FS syscalls call `ext4_*` directly. Primary blocker to Plan 9 / seL4-service goals. |
| EXT4-02 | W4 | SECURITY | `core/syscall_dispatch.c:176-199` | `FILE_WRITE` has no access control — any PID can overwrite any file, including `/init`. |
| EXT4-03 | W4 | DOC·BUG | `fs/ext4.c:3,234` | "Read-Only" header is false: `ext4_write_file` really persists to disk (capped 48 KB / 12 direct blocks). |

(Full W3 table and the W2/W1/W0 detail live in the per-subsystem docs; the W3 set is the
remaining actionable tier and is the basis for the issue batch — see §6.)

---

## 5. Corrections & reconciliations (review integrity)

Maintainer spot-checks of agent-delegated analysis corrected the following before anything
was published — recorded for transparency:

- **My own early errors, retracted:** (a) I claimed init's supervisor loop was "broken
  (blocking wait)" — wrong; `process_wait` is non-blocking, the loop is a valid poll.
  (b) I claimed `draw3d.c` "doesn't exist" — it exists but is **not compiled** (orphan).
- **Reconciled inference:** the amd64 4G crash I first attributed to MMIO-mapping range
  (AMMU-07) is actually **DRV-VIRTIO-01** (64-bit BAR truncation), confirmed by source.
  AMMU-07 downgraded to a lesser, separate MMIO-coverage limitation.
- **Agent overstatements caught & downgraded:** USR-INIT-01 "PID-reuse W3" → **W1**
  (`next_pid` is monotonic, no reuse). USR-MALLOC-01 calloc mechanism reworded (the
  `memset` is in-bounds; the *caller* overflows). LIB-MATH-01 "all 3D corrupted W3" →
  **W2 latent** (only consumer `draw3d.c` is not compiled).
- **New cross-cutting finding surfaced:** orphaned/uncompiled source files exist in-tree
  (`graphics/draw3d.c`, `bin/test_init.c`, the dead `user/sys/lib/syscall.S`) — dead code.

Provenance: docs **01, 02, 09** hand-written by maintainer; **03–08** agent-generated and
maintainer spot-checked (top/critical findings verified against source; **07, 08** carry
explicit correction notes). Every finding cites `file:line`.

---

## 6. Per-subsystem index

| Doc | Subsystem | Notes |
|---|---|---|
| [01-mm](analysis/01-mm-memory-management.md) | PMM/VMM/buffer/kmalloc | maintainer |
| [02-boot-arch-hal](analysis/02-boot-arch-hal.md) | boot, platform, HAL, arch-MMU | maintainer |
| [03-arch-cpu-exceptions](analysis/03-arch-cpu-exceptions.md) | CPU/IDT/exceptions/syscall-entry/uaccess | agent, vetted |
| [04-drivers-irq](analysis/04-drivers-irq.md) | virtio/gpu/uart/gic/timer/pci/irq | agent, vetted (W5 here) |
| [05-fs](analysis/05-fs.md) | vfs/ext4/gpt | agent, vetted (2×W5) |
| [06-graphics](analysis/06-graphics.md) | compositor/font/gl/region | agent, vetted |
| [07-lib-headers](analysis/07-lib-headers.md) | kernel lib + ABI headers + registry | agent, vetted+corrected |
| [08-userland](analysis/08-userland.md) | init/shell/services/libs/apps | agent, vetted+corrected |
| [09-sched-process-ipc-abi](analysis/09-sched-process-ipc-abi.md) | scheduler/process/IPC/ABI | maintainer |

## 7. Issues (GitHub)

The W3+ actionable tier (**72 findings**) is filed as individual GitHub issues on
`olmox001/os1test-dev`, labeled by severity (`w3`/`w4`/`w5`), kind (`bug`, `security`,
`wrong-design`, `missing`, `stub`, `bad-impl`, `refine`, `perf`, `review-doc`) and
`area:*`, all tagged `code-review`. W0–W2 findings remain in the per-subsystem docs above.

- **Tracking epic:** [#19](https://github.com/olmox001/os1test-dev/issues/19)
- **Per-finding issues:** [#20–#91](https://github.com/olmox001/os1test-dev/issues?q=is%3Aissue+is%3Aopen+label%3Acode-review) (72)
- **Cross-cutting epics:** #92 Memory & address-space · #93 ABI & capabilities ·
  #94 amd64 boot/4GB · #95 Service isolation (seL4/Plan 9) · #96 SMP correctness
- Filter examples: `gh issue list --label code-review`, `--label w5`, `--label area:fs`.

Each issue body carries the `file:line` location, the finding text (maintainer-corrected),
and a pointer to its subsystem doc + this index. They are the unit of work for the
delegated fix phase (Phase 3).

## 8. Phase 3 — fixes landed (branch `comprehensive-review`)

Each verified by build (both arches) + headless QEMU runtime, committed separately,
delegated one-agent-at-a-time and maintainer-verified before commit.

| Commit | Fix | Issue |
|---|---|---|
| `0c5dc0a` | amd64 read full 64-bit PCI BAR (virtio.c + hal.c) + `arch_vmm_map_device` | **#44** (W5) ✅ |
| `89c3a52` | amd64 clone high device-MMIO PML4 entries into process PGDs (fixes ≥4G `0xc0…` fault) | part of #94 ✅ |
| `fedd9e2` | amd64 detect PVH via `hvm_start_info.magic` → real memory map (up to 4GB+) | **#28, #29** ✅ |
| `8b03255` | amd64 `*(.lbss*)`→`.bss` so PMM metadata no longer overlaps `cpu_data` (SMP `current_task` page-fault) | runtime-discovered ✅ |
| `b3ea74f` | aarch64 real DTB via `-dtb`/raw `kernel.bin` (FDT works, `x0` set) + SMP fallback cap 64→8; `-m 5G` default both arches | runtime-discovered ✅ |

**Verified runtime status now:** amd64 boots clean at `-m 3G / 5G / 8G` (detects 6–9 GB,
virtio-blk + Ext4, 4 SMP cores, no faults); aarch64 FDT-driven (real RAM + CPU count),
boots to the TTY shell.

**Remaining (open):** IPC→64-bit (not started — the delegated agent stopped before
editing; the system works without it); amd64 ACPI-MADT CPU count (ARCH-01), real
PCI/ACPI (ARCH-02), user-vs-kernel fault isolation (EXC-AMD64-02); the kernel/userland
higher-half **addressing rework**; and re-commenting the headers + `.S` files that were
reverted in Phase 2 (the C sources are commented and committed).
