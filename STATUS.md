# OS1 Microkernel Refactoring Status

## Project Scope
Transition OS1 from a monolithic kernel to a pure microkernel architecture.
- **Kernel**: Minimalist, handles only IPC, Scheduling, PMM/VMM, and HAL.
- **Services**: VFS, Compositor, Drivers, and Shell run as isolated user-space processes.
- **ABI**: Unified, IPC-centric interface for all system requests.

## Current Phase: Phase 2 - User-space Service Bootstrap

### Sub-Phase 2.1: Kernel IPC & Init Process [/]
- [x] Phase 1: The Great Reset (Base Layering)
- [/] Implement blocking IPC primitives in kernel (`sched/process.c`)
- [ ] Create initial user-space `init` process (`user/sys/bin/init.c`)
- [ ] Establish communication channel between Kernel and `init`

### Sub-Phase 2.2: VFS Service Migration [ ]
- [ ] Refactor `vfs.c.old` into `user/sys/bin/vfs`
- [ ] Define VFS IPC protocol (OPEN, READ, WRITE, CLOSE, REaddir)
- [ ] Implement VFS namespace virtualization (per-process roots)

### Sub-Phase 2.3: HAL & User-space Drivers [ ]
- [ ] Implement `hal_device` mapping for user-space
- [ ] Port UART/Timer drivers to user-space
- [ ] Port VirtIO drivers to user-space

## Architecture Mapping
- **HAL**: `kernel/arch/`
- **Core**: `kernel/core/`
- **Libkernel**: `kernel/libkernel/`
- **User Sys**: `user/sys/` (Core services)
- **User Bin**: `user/bin/` (Applications)

## Build Status
- **AArch64**: [OK] (Minimal Kernel)
- **AMD64**: [OK] (Minimal Kernel)
