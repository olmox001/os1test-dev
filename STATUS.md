# Stato del Refactoring del Microkernel OS1 (Mainbrain Totale)

Questo documento traccia lo stato reale e la situazione corrente del refactoring del microkernel OS1, allineato alle direttive di progettazione senior (ispirate ai paradigmi Plan 9, seL4 e Mach4) e alla transizione open-source GPLv2.

---

## ⚖️ Licenza & Allineamento Open-Source
Il microkernel OS1 è licenziato sotto la **GNU General Public License, Version 2 (GPLv2)**. Questo allinea pienamente il progetto alla licenza del kernel **Linux**, garantendo libertà di modifica e condivisione.

### Fonti e Riferimenti Integrati nel Codebase (In Ordine di Riferimento):
1.  **Plan 9 (Pilastri Principali)**: Registro gerarchico dinamico con code ad anello per IPC serializzato in [registry.c](kernel/libkernel/src/registry.c).
2.  **seL4 (Pilastri Principali)**: Thinned HAL per confinare i vettori assembly in [exception.S](kernel/hal/arch/aarch64/cpu/exception.S) (AArch64) e [start.S](kernel/hal/arch/amd64/boot/start.S) (AMD64), isolando lo stato del contesto `pt_regs`.
3.  **Linux (Kernel)**: Liste intrinseche doppiamente concatenate in [list.h](kernel/core/include/core/list.h), parser a blocchi Ext4 in [ext4.c](kernel/core/src/fs/ext4.c) e GPT in [gpt.c](kernel/core/src/fs/gpt.c).
4.  **Progetto base-nexs**: Coordinazione dei registri globali integrata in [registry.c](kernel/libkernel/src/registry.c).
5.  **FreeBSD / BSD**: Architettura VFS a nodi virtuali (vnode) e path translation (`namei`) in fase di integrazione tramite l'interfaccia [vfs.h](kernel/core/include/core/vfs.h).
6.  **Mach4**: Code di messaggi asincrone per IPC in [syscall.c](kernel/core/src/syscall.c).
7.  **Libreria di Font (stb_truetype & stb_easy_font)**: Rasterizzazione tipografica per visualizzazione grafica, integrato in [stb_truetype.h](tools/stb_truetype.h) e [stb_easy_font.h](user/sys/include/stb_easy_font.h).
8.  **Limine**: Struttura dei bootloader stage in [kernel/hal/boot/](kernel/hal/boot/).

---

## 1. Stato del File Stub (`stubs.c`)
Il file [stubs.c](kernel/core/src/stubs.c) è **completo e corretto**.
- In linea con la direttiva di **mantenere il compositore e il mount residenti nel kernel**, il compositore reale ([compositor.c](kernel/core/src/graphics/compositor.c)) è integrato e compilato perfettamente.
- Gli stub duplicati per `compositor_*` sono stati **eliminati** da `stubs.c` per evitare conflitti di definizione multipla del linker.
- Rimangono in `stubs.c` solo i reali stub necessari per le chiamate VFS non ancora migrate (`sys_open`, `sys_read`, `sys_write`, `sys_close`).

---

## 2. Stato delle Fasi del Progetto

### 🟢 Fase 1: HAL Decoupling, Relocation, e Thinning (COMPLETATO)
- **Bootstrap e Userland Relocation**: Tutti i percorsi assembly obsoleti (`boot/` e `user/`) sono stati rimossi dalla root del progetto e riorganizzati sotto i moduli strutturati della HAL:
  - [kernel/hal/boot/aarch64/](kernel/hal/boot/aarch64/)
  - [kernel/hal/boot/amd64/](kernel/hal/boot/amd64/)
  - [kernel/hal/user/](kernel/hal/user/)
- **Makefile Hardening**: Il build system è stato esteso per mappare i nuovi percorsi in modo pulito e robusto per entrambe le architetture.
- **Rimozione File Obsoleti**: Abbiamo rimosso fisicamente e dal tracciamento git il file `user/sys/lib/syscall.S` per garantire la massima pulizia del codice utente.

### 🟢 Fase 2: Driver Decoupling & MMIO / PCI Decoupling (COMPLETATO)
- **Riorganizzazione Fisica**: I driver (UART, GIC/PIC, VirtIO block, VirtIO GPU, Keyboard) sono suddivisi ordinatamente sotto [kernel/hal/drivers/mmio/](kernel/hal/drivers/mmio/) e [kernel/hal/drivers/pci/](kernel/hal/drivers/pci/).
- **Protocollo a Messaggi**: Abbandonato il legame funzionale C diretto. I driver espongono una porta ed un callback asincrono di `dispatch()` all'interno del bus dei driver gestito da [drivers.c](kernel/core/src/drivers.c).
- **Driver Portati**: UART PL011, UART 16550, e VirtIO-Block registrano ed eseguono correttamente i propri comandi tramite messaggi IPC strutturati (`REG_MSG_MMIO_WRITE`, `REG_MSG_MMIO_READ`, `REG_MSG_BLK_READ`/`REG_MSG_BLK_WRITE`).

### 🟡 Fase 3: Registro Gerarchico Plan 9 + seL4 & Integrazione VFS (In Corso)
- **Cosa è fatto**: Registro gerarchico dinamico e flessibile (`RegKey` / `RegIpcQueue`) con code ad anello gestito tramite IPC nativo in [registry.c](kernel/libkernel/src/registry.c). Le syscall IPC associate sono attive.
- **Da Farsi**: Posizionare il registro gerarchico stile Plan 9 come pilastro fondante *al di sotto* di VFS e Compositore. Montare l'albero di registro su `/sys/registry` consentendo a VFS, Compositatore, e futuri stack di rete di comunicare ed effettuare l'I/O con i driver tramite normali file.

### 🟡 Fase 4: Header Synchronization & Cleaning (Pianificato)
- **Cosa è fatto**: Avviata la pulizia dei file doppi in `user/sys/include/sys`.
- **Cosa manca**: Il caricatore ELF del kernel ([elf.c](kernel/core/src/sched/elf.c)) include ancora `elf.h` pescato dal percorso `user/`. Questa è una violazione del confine kernel/userland.
- **Da Farsi**: Separare nettamente gli header. Creare `kernel/core/include/core/elf.h` ad uso esclusivo del kernel, in modo che il kernel non dipenda in alcun modo dagli header userland.

---

## 3. Stato di Compilazione e Test
- **AArch64**: `make clean && make all ARCH=aarch64` ➔ **🟢 OK** | `make test-release` ➔ **🟢 OK (Boot Stabile e Shell Attiva)**
- **AMD64**: `make clean ARCH=amd64 && make all ARCH=amd64` ➔ **🟢 OK** | `make test-release ARCH=amd64` ➔ **🟢 OK (Boot Stabile e Shell Attiva via MBR Fallback)**
