# MICROSCOPE — Release, boot & storage rework

> A microscopic, evidence-based plan for the release/boot/storage phase.
> Method: **ASTRA** (`docs/ASTRA.md`) — kernel core consumes contracts;
> every hardware/format is a *provider* behind a contract; arch dirs hold
> only ISA/boot-protocol glue.  Branch `comprehensive-review`.
> Paused work is archived in `docs/B3-POLISH-QUEUE.md`.

## 1. The bug (reproduced, root-caused)

`make ARCH=amd64 release-arch` builds a GRUB El-Torito ISO that boots the
kernel via `multiboot2 /boot/kernel.elf` and passes the rootfs as
`module2 /boot/disk.img diskimg`.  Booting the ISO (`-cdrom`, headless):

```
[INFO] VirtIO: No block device found
[INFO] Partition: Failed to read LBA 1
[INFO] GPT: Done.
[ERROR] VFS: no mountable filesystem found on any partition
[ERROR] ELF: File not found: /sys/bin/init
*** KERNEL PANIC ***  Failed to load /init
```

**Root cause:** the released ISO is self-contained, but the kernel only ever
looks for the rootfs on a **virtio-blk GPT/ext4 block device**.  In the ISO
boot there is none — the rootfs (`disk.img`) is a **multiboot2 module in RAM**
that the kernel never reads (`platform.c` counts `nr_modules` but does not
parse the MODULE tag).  So GPT finds nothing, VFS mounts nothing, `/sys/bin/init`
is absent → panic.  Exactly the reported "block device / disk.img partitions /
poorly-supported PCI hardware" failure.

Secondary issues observed:
- The release `dd`-zeroes the ISO's first 512 bytes ("to be readable from
  macOS"), which **destroys the hybrid MBR** → the ISO no longer boots as a
  hard disk (`virtio-blk`), only as El-Torito CD.  `test-release` then attaches
  it as `virtio-blk-pci`, which cannot boot → blank serial.
- `disk.img` is built by `mkdisk` as **bootloader + kernel + rootfs** (GPT) —
  the kernel is **redundant** in the module path (GRUB already loaded
  `kernel.elf`); it only bloats the module.

## 2. Target architecture (ASTRA)

The fix is to make the **GRUB module a block provider**, so the *existing*
GPT→ext4→VFS path mounts it with zero changes to the FS stack:

```
GRUB ──multiboot2──> kernel.elf
     └─module2──────> disk.img (standard GPT+ext4 rootfs, in RAM)
                          │
   platform (arch glue): parse MB2 MODULE tag → (mod_start, mod_end)
                          │
   ramdisk block provider (RAM-backed) registered behind the BLOCK contract
                          │
   GPT probe → ext4 mount → VFS root  ← unchanged, already works
```

Layering (simplicity at the centre, complexity at the edges):
- **Arch boot glue** (`arch/amd64/platform`): parse the multiboot2 MODULE tag,
  hand `(base,size)` to the core via a contract call.  aarch64 has no GRUB; it
  keeps the virtio-blk path (and later a DT/`-initrd` equivalent).
- **Block contract**: add a **ramdisk** block device (read-only, RAM-backed)
  that the GPT/buffer-cache layer consumes exactly like virtio-blk.  The kernel
  core never learns "module vs disk".
- **FS providers** behind the VFS contract: keep **ext4** (read path), add
  **tmpfs** (RAM-backed, for `/tmp`, `/sys/log`, runtime state), and add
  **xfs** as the on-disk fs for real persistence.  VFS already has
  `fs_ops`+mount table (B1) — these slot in as providers.
- **Memory drivers**: extend the PMM/region layer so a RAM-backed disk and
  tmpfs share one accounting path (reserve the module region, expose it as a
  zone the ramdisk/tmpfs allocate from).

## 3. Phased plan

**R1 — standard disk.img + ramdisk module boot (fixes the panic).**
1. `mkdisk`/Makefile: build a **rootfs-only** `disk.img` (drop the redundant
   bootloader+kernel partitions from the release image; keep GPT+ext4 so the
   block path is unchanged).  `disk.img` becomes a standard data image.
2. `arch/amd64/platform`: parse MB2 tag type 3 (MODULE) → store
   `rootfs_mod_base/size`; reserve the region in the memmap so the PMM does
   not hand it out.
3. Core: a **ramdisk block provider** over `[base,size)`, registered when a
   module is present; GPT/ext4/VFS mount it.  Block-device probe order:
   virtio-blk first, ramdisk fallback (so `make run` keeps using virtio-blk,
   the ISO uses the module).
4. Fix the release: stop `dd`-zeroing the MBR (keep the ISO bootable as CD);
   `test-release` boots via `-cdrom` (El-Torito), no virtio-blk drive needed.
   Acceptance: the ISO boots to the shell on both `-cdrom` and (hybrid) HDD.

**R2 — tmpfs.** RAM-backed fs provider behind VFS; mount `/tmp` and
`/sys/log` (the per-process `log <pid>` tee from queue item 2 writes here).
No disk needed; uses the extended memory path.

**R3 — xfs (disk persistence).** Add an xfs read/write provider behind VFS for
real on-disk storage; ext4 stays for the legacy/rootfs image.  (Large — its
own sub-plan; allocation groups, B+trees, log.)

**R4 — memory drivers.** Unify RAM-disk + tmpfs + PMM zones; growable
reservations; document the memory map per arch.

## 4. Constraints & notes
- aarch64 toolchain pinned (GCC 7.2.0); amd64 uses `x86_64-elf-`.
  `i686-elf-grub-mkrescue` is at `/usr/local/bin`.
- Headless release test: `-cdrom <iso>` (NOT `-drive ... virtio-blk` after the
  MBR is zeroed).  Kernel serial appears once GRUB hands off (multiboot2).
- Keep `make run` (dev) on virtio-blk + `disk.img` so the daily loop is
  unchanged; only the **release** path moves to the module/ramdisk.
- ASTRA: no FS/format code in `arch/`; the module parse is the only arch glue,
  everything else is a provider behind the block/VFS contract.

## 5. Status
- R1 step 1–4: **planned** (this doc).  Diagnosis verified 2026-06-13.
- Paused B3-polish queue: see `docs/B3-POLISH-QUEUE.md` (resume after R1–R4).
