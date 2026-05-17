# Stato del Refactoring del Microkernel OS1 (Mainbrain Totale)

Questo documento traccia lo stato reale e la situazione corrente del refactoring del microkernel OS1, allineato alle direttive di progettazione senior (ispirate ai paradigmi Plan 9, seL4 e Mach4) e alla transizione open-source GPLv2.

---

## ⚖️ Licenza & Allineamento Open-Source
Il microkernel OS1 è licenziato sotto la **GNU General Public License, Version 2 (GPLv2)**. Questo allinea pienamente il progetto alla licenza del kernel **Linux**, garantendo libertà di modifica e condivisione.

### Fonti e Riferimenti Integrati nel Codebase (In Ordine di Priorità):
1.  **Plan 9 (Pilastri Principali)**: Registro gerarchico dinamico con code ad anello per IPC serializzato implementato in [registry.c](file:///Users/olmo/Documents/git/ostest1/os1test-dev/kernel/libkernel/src/registry.c).
2.  **seL4 (Pilastri Principali)**: Thinned HAL per confinare i vettori assembly in [exception.S](file:///Users/olmo/Documents/git/ostest1/os1test-dev/kernel/hal/arch/aarch64/cpu/exception.S) (AArch64) e [start.S](file:///Users/olmo/Documents/git/ostest1/os1test-dev/kernel/hal/arch/amd64/boot/start.S) (AMD64), isolando lo stato del contesto `pt_regs`.
3.  **Linux (Kernel)**: Modello di liste intrinseche doppiamente concatenate in [list.h](file:///Users/olmo/Documents/git/ostest1/os1test-dev/kernel/core/include/core/list.h), parser a blocchi Ext4 in [ext4.c](file:///Users/olmo/Documents/git/ostest1/os1test-dev/kernel/core/src/fs/ext4.c) e GPT in [gpt.c](file:///Users/olmo/Documents/git/ostest1/os1test-dev/kernel/core/src/fs/gpt.c).
4.  **Progetto base-nexs**: Modello e protocolli di coordinazione dei registri globali integrati in [registry.c](file:///Users/olmo/Documents/git/ostest1/os1test-dev/kernel/libkernel/src/registry.c).
5.  **FreeBSD / BSD**: Architettura VFS a nodi virtuali (vnode) e path translation (`namei`) in fase di integrazione tramite l'interfaccia [vfs.h](file:///Users/olmo/Documents/git/ostest1/os1test-dev/kernel/core/include/core/vfs.h).
6.  **Mach4**: Code di messaggi asincrone per IPC in [syscall.c](file:///Users/olmo/Documents/git/ostest1/os1test-dev/kernel/core/src/syscall.c).
7.  **Libreria di Font (stb_truetype & stb_easy_font)**: Motore di rasterizzazione tipografica ad altissima efficienza di Sean Barrett per visualizzazione grafica, integrato in [stb_truetype.h](file:///Users/olmo/Documents/git/ostest1/os1test-dev/tools/stb_truetype.h) e [stb_easy_font.h](file:///Users/olmo/Documents/git/ostest1/os1test-dev/user/sys/include/stb_easy_font.h).
8.  **Engine DoomGeneric**: Integrazione per la portabilità rapida di applicazioni e giochi interattivi su framebuffer personalizzato.
9.  **Limine**: Struttura dei bootloader stage in [kernel/hal/boot/](file:///Users/olmo/Documents/git/ostest1/os1test-dev/kernel/hal/boot/).

---

## 1. Stato del File Stub (`stubs.c`)

Il file [stubs.c](file:///Users/olmo/Documents/git/ostest1/os1test-dev/kernel/core/src/stubs.c) è **completo e corretto**.
- In linea con la direttiva di **mantenere il compositore e il mount residenti nel kernel**, il compositore reale ([compositor.c](file:///Users/olmo/Documents/git/ostest1/os1test-dev/kernel/core/src/graphics/compositor.c)) è integrato e compilato perfettamente.
- Gli stub duplicati per `compositor_*` sono stati **eliminati** da `stubs.c` per evitare conflitti di definizione multipla del linker.
- Rimangono in `stubs.c` solo i reali stub necessari per le chiamate VFS non ancora migrate (`sys_open`, `sys_read`, `sys_write`, `sys_close`).

---

## 2. Stato delle Fasi del Progetto

### 🟢 Fase 1: HAL Decoupling, Relocation, e Thinning (COMPLETATO)
- **Bootstrap e Userland Relocation**: Tutti i percorsi assembly obsoleti (`boot/` e `user/`) sono stati rimossi dalla root del progetto e riorganizzati sotto i moduli strutturati della HAL:
  - [kernel/hal/boot/aarch64/](file:///Users/olmo/Documents/git/ostest1/os1test-dev/kernel/hal/boot/aarch64/)
  - [kernel/hal/boot/amd64/](file:///Users/olmo/Documents/git/ostest1/os1test-dev/kernel/hal/boot/amd64/)
  - [kernel/hal/user/](file:///Users/olmo/Documents/git/ostest1/os1test-dev/kernel/hal/user/)
- **Makefile Hardening**: Il build system è stato esteso per mappare i nuovi percorsi in modo pulito e robusto per entrambe le architetture.
- **Rimozione File Obsoleti**: Abbiamo rimosso fisicamente e dal tracciamento git il file `user/sys/lib/syscall.S` (un assembly duplicato e obsoleto della vecchia codebase AArch64) per garantire la massima pulizia del codice utente.

### 🟡 Fase 2: Driver Decoupling & MMIO / PCI Decoupling (In Corso - Avvio Pianificazione)
- **Cosa è fatto**: I driver fisici (UART, GIC/PIC, VirtIO block, VirtIO GPU, Keyboard) sono posizionati sotto [kernel/hal/drivers/](file:///Users/olmo/Documents/git/ostest1/os1test-dev/kernel/hal/drivers/).
- **Cosa manca**: I driver espongono ancora funzioni C dirette altamente accoppiate con il kernel.
- **Da Farsi**: 
  - Raggruppare strutturalmente i driver in `drivers/mmio/` e `drivers/pci/`.
  - Implementare l'interfaccia a messaggi stile seL4 in cui ogni driver registra una porta IPC all'interno del registro di sistema, gestendo l'I/O attraverso l'invio di messaggi IPC strutturati.

### 🟢 Fase 3: Registro Gerarchico Plan 9 + seL4 (Completato al 90%)
- **Cosa è fatto**: È stato implementato e integrato un registro gerarchico dinamico e flessibile (`RegKey` / `RegIpcQueue`) con code ad anello gestite tramite IPC nativo in [registry.c](file:///Users/olmo/Documents/git/ostest1/os1test-dev/kernel/libkernel/src/registry.c). Le syscall IPC associate (`SYS_REG_IPC_SEND`, `SYS_REG_IPC_RECV`, `SYS_REG_IPC_PEND`, `SYS_REG_IPC_LIST`) sono attive.
- **Da Farsi**: Sostituire le restanti costanti hardcoded nei driver con query dinamiche effettuate sull'albero di registro `/sys/drivers/` popolato durante la fase di boot.

### 🟡 Fase 4: Header Synchronization & Cleaning (Pianificato)
- **Cosa è fatto**: È stata avviata la pulizia dei file doppi in `user/sys/include/sys`.
- **Cosa manca**: Il caricatore ELF del kernel ([elf.c](file:///Users/olmo/Documents/git/ostest1/os1test-dev/kernel/core/src/sched/elf.c)) include ancora `elf.h` pescato dal percorso `user/`. Questa è una violazione del confine kernel/userland.
- **Da Farsi**: Separare nettamente gli header. Creare `kernel/core/include/core/elf.h` ad uso esclusivo del kernel, in modo che il kernel non dipenda in alcun modo dagli header userland.

### 🟢 Fase 5: Restore Kernel-Resident VFS e Compositor & Syscalls (COMPLETATO)
- **Cosa è fatto**: Il compositore grafico reale ([compositor.c](file:///Users/olmo/Documents/git/ostest1/os1test-dev/kernel/core/src/graphics/compositor.c)) e il caricamento del file system tramite codice a blocchi indiretti di [boot_fs.c](file:///Users/olmo/Documents/git/ostest1/os1test-dev/kernel/core/src/boot_fs.c) sono residenti nel kernel. Le chiamate di sistema grafiche e di I/O sono completamente instradate. Il sistema si avvia e apre la shell in una finestra del compositore in modo totalmente stabile su entrambe le architetture.
- **Memory Hardening**: Protezione a livello MMU per forzare l'isolamento assoluto delle pagine utente (impedendo la lettura/scrittura di memoria del kernel da user-space) integrata stabilmente.

### 🟢 Fase 6: Multi-Arch Build & Verification (COMPLETATO)
- **Stato AArch64**: Compila perfettamente e si avvia in QEMU, eseguendo con successo `init`, il server di notifiche e la shell interattiva senza alcun crash o eccezione (`make test-release` ➔ 🟢 **SUCCESS**).
- **Stato AMD64**: Compila e si avvia stabilmente sia via boot diretto (`make run`) sia in modalità di rilascio tramite ISO ibrida (`make test-release ARCH=amd64`).
- **Risoluzione stabilità AMD64**:
  1. **Boot Initialization Order**: Abbiamo anticipato la chiamata a `cpu_init()` prima di `arch_platform_early_init()` per garantire che le tabelle dei descrittori (GDT, IDT) e i vettori di eccezione siano registrati molto presto nel ciclo di vita del kernel. Questo ci permette di intercettare e visualizzare nel dettaglio qualsiasi eccezione iniziale (Page Fault, GPF) invece di incorrere in un triple fault con reset silente.
  2. **Robust MBR Fallback for ISO Boot**: Durante l'avvio tramite ISO ibrida (usata in `test-release` su AMD64 via GRUB), l'immagine del blocco caricato non presenta una tabella delle partizioni GPT a causa della generazione ibrida. Abbiamo implementato un meccanismo di fallback robusto in [boot_fs.c](file:///Users/olmo/Documents/git/ostest1/os1test-dev/kernel/core/src/boot_fs.c): se la firma GPT non è valida, scansioniamo la tabella delle partizioni MBR cercando una partizione nativa Linux (tipo `0x83`). Se trovata, montiamo la partizione Ext4 di userland con successo.

---

## 3. Stato di Compilazione e Test
- **AArch64**: `make clean && make all ARCH=aarch64` ➔ **🟢 OK** | `make test-release` ➔ **🟢 OK (Boot Stabile e Shell Attiva)**
- **AMD64**: `make clean ARCH=amd64 && make all ARCH=amd64` ➔ **🟢 OK** | `make test-release ARCH=amd64` ➔ **🟢 OK (Boot Stabile e Shell Attiva via MBR Fallback)**
