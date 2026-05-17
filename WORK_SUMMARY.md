# Work Summary — Microkernel Reorganization (Commit a78e99d → test)

## Contesto

Continuazione della sessione precedente che ha eseguito la riorganizzazione completa in 5 fasi
del kernel (commit `a78e99d`). Obiettivo corrente: testare su QEMU (AArch64 + AMD64) e
correggere i bug di runtime scoperti.

---

## Bug corretti nella sessione precedente (prima del riassunto)

Tutti e tre i bug erano in `kernel/core/src/main.c` — il nuovo `main.c` della riorganizzazione
non chiamava alcune funzioni di inizializzazione presenti nel vecchio file.

| # | Bug | File | Fix |
|---|-----|------|-----|
| 1 | GIC CPU interface disabilitato — nessun IRQ consegnato | `main.c` | Aggiunto `irq_init_percpu()` dopo `irq_init()` |
| 2 | Timer mai armato — `CNTV_CTL_EL0 = 0` | `main.c` | Aggiunto `driver_timer_init()` + `timer_init_percpu()` |
| 3 | Nessun idle task — scheduler senza task da eseguire | `main.c` | Aggiunto `smp_create_idle_task(0)` dopo `process_init()` |

---

## Lavoro svolto in questa sessione

### 1. Diagnostiche aggiunte in `kernel/hal/arch/aarch64/cpu/syscall.c`

Aggiunto nel ramo `if (ec != 0x15)` di `syscall_handler` (dopo riga 190), per discriminare
le cause di crash EC=0x0:

```c
pr_err("SPSR=0x%lx (M[3:0]=%lu EL%lu)\n", ...);
pr_err("TTBR0=0x%lx proc->page_table=0x%lx\n", ...);
// + dump bytes@ELR se ELR in range utente
```

### 2. Build AArch64 — SUCCESSO

```
make clean ARCH=aarch64 && make ARCH=aarch64 all
```
Build completata senza errori.

### 3. Test AArch64 — FUNZIONA ✓

Output QEMU (rilevante):
```
[C0] [INFO] Microkernel: Spawning Init (PID 2)
[Init] System Initialization Starting...
[Init] Spawning Notification Server...
[C0] [INFO] Syscall: SPAWN /sys/bin/notify_srv
[C0] [INFO] process_create: 'notify_srv' PID=3
[C0] [INFO] Compositor: Created window 'Notifiche' (250x60) at (460,10)
[Notify] Server started (PID 3)
[Init] Spawning Shell...
[C0] [INFO] Syscall: SPAWN /sys/bin/shell
Shell: Alive
[C0] [INFO] Compositor: Created window 'Shell PID 4' (640x480)
[Shell] TTY Window active (PID 4).
shell:/> 
```

**Init → notify_srv → shell: tutti funzionanti.**
Il crash EC=0x0 della sessione precedente era causato dai tre bug di init. Le diagnostiche
non hanno prodotto output perché il sistema ora funziona correttamente.

### 4. Test AMD64 — FALLISCE (triple fault / reboot loop)

Output QEMU (si ripete in loop):
```
[C0] [INFO] Console driver initialized
[C0] [INFO] AMD64 Platform Initialization (Magic: 0x0, Info: 0x1580)
[C0] [INFO] IRQ: Registered chip 8259 PIC
[C0] [INFO] PIC Initialized and remapped to 32-47.
[C0] [WARN] AMD64: [IDTF] Unknown boot protocol (Magic: 0x0). Using safe 1GB default.
→ RESET (SeaBIOS di nuovo)
```

**Causa**: Triple fault che avviene dopo `arch_platform_early_init()` e prima della stampa
del banner. Il kernel viene caricato via `-kernel` (direct boot QEMU) che non fornisce
Multiboot2 magic (0x36d76289), quindi mb_magic = 0x0.

Il crash avviene probabilmente in `cpu_init()` → `arch_cpu_init()` → `gdt_init()` o
`lapic_init()` prima ancora del primo `pr_info` di quella funzione. Analisi ancora in corso
al momento dell'interruzione.

**File da investigare**:
- `kernel/hal/arch/amd64/cpu/cpu.c` — `arch_cpu_init()` (GDT/IDT/LAPIC)
- `kernel/hal/arch/amd64/cpu/gdt.c` — potenziale causa di triple fault
- `kernel/hal/arch/amd64/boot/start.S` — stato paging/stack al momento di `kernel_main`

---

## Strategia VFS/Compositor (Fase 5) — Confermata

Il commit `a78e99d` ha rimosso `vfs.c` e `compositor.c` dal kernel (architettura microkernel).
La strategia di porting asincrono è corretta:
- Sorgenti VFS recuperabili da: `git show 7218e30:kernel/fs/vfs.c`
- Implementazione BSD VFS disponibile nel branch `test-stable-reload`
- Destinazioni già create: `user/sys/bin/vfs/` e `user/sys/bin/compositor/`
- Da fare nella Fase 5: compilare come ELF utente, caricare da init, sostituire chiamate
  kernel con code IPC via registry queue

---

## Stato corrente

| Componente | Stato |
|-----------|-------|
| AArch64 build | ✓ OK |
| AArch64 boot (init/shell/notify) | ✓ OK |
| AMD64 build | ✓ OK |
| AMD64 boot | ✗ Triple fault dopo early init |
| VFS daemon (Fase 5) | Pendente |
| Compositor daemon (Fase 5) | Pendente |

---

## Prossimi passi

1. **Debuggare AMD64**: aggiungere `pr_err` in `arch_cpu_init()` prima di `gdt_init()`,
   oppure usare `make ARCH=amd64 debug` + GDB per trovare l'indirizzo del triple fault.
2. **Fase 5**: estrarre VFS e compositor, portarli come daemon utente.
3. **Commit**: una volta che AMD64 funziona, fare commit con tutti i fix.
