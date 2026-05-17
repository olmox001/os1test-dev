# Stato del Refactoring del Microkernel OS1 (Mainbrain Totale)

Questo documento traccia lo stato reale e la situazione corrente del refactoring del microkernel OS1, allineato alle direttive di progettazione senior (ispirate ai paradigmi Plan 9, seL4 e Mach4).

---

## 1. Stato del File Stub (`stubs.c`)

Il file `kernel/core/src/stubs.c` è **completo e corretto**.
- In linea con la direttiva di **mantenere il compositore e il mount residenti nel kernel**, il compositore reale (`kernel/core/src/graphics/compositor.c`) è integrato e compilato perfettamente.
- Gli stub duplicati per `compositor_*` sono stati **eliminati** da `stubs.c` per evitare conflitti di definizione multipla del linker.
- Rimangono in `stubs.c` solo i reali stub necessari per le chiamate VFS non ancora migrate (`sys_open`, `sys_read`, `sys_write`, `sys_close`).

---

## 2. Stato delle Fasi del Progetto

### 🟢 Fase 1: HAL Decoupling, Relocation, e Thinning (COMPLETATO)
- **Bootstrap e Userland Relocation**: Tutti i percorsi assembly obsoleti (`boot/` e `user/`) sono stati rimossi dalla root del progetto e riorganizzati sotto i moduli strutturati della HAL:
  - `kernel/hal/boot/aarch64/`
  - `kernel/hal/boot/amd64/`
  - `kernel/hal/user/`
- **Makefile Hardening**: Il build system è stato esteso per mappare i nuovi percorsi in modo pulito e robusto per entrambe le architetture.
- **HAL Thinning**: Tutte le strutture e i registri primitivi sono stati confinati rigidamente all'interno dello strato HAL.

### 🟡 Fase 2: Driver Decoupling & MMIO / PCI Decoupling (Pianificato)
- **Cosa è fatto**: I driver fisici (UART, GIC/PIC, VirtIO block, VirtIO GPU, Keyboard) sono posizionati sotto `kernel/hal/drivers/`.
- **Cosa manca**: I driver espongono ancora funzioni C dirette altamente accoppiate con il kernel.
- **Da Farsi**: 
  - Raggruppare strutturalmente i driver in `drivers/mmio/` e `drivers/pci/`.
  - Implementare l'interfaccia a messaggi stile seL4 in cui ogni driver registra una porta IPC all'interno del registro di sistema, gestendo l'I/O attraverso l'invio di messaggi IPC strutturati.

### 🟢 Fase 3: Registro Gerarchico Plan 9 + seL4 (Completato al 90%)
- **Cosa è fatto**: È stato implementato e integrato un registro gerarchico dinamico e flessibile (`RegKey` / `RegIpcQueue`) con code ad anello gestite tramite IPC nativo in `kernel/libkernel/src/registry.c`. Le syscall IPC associate (`SYS_REG_IPC_SEND`, `SYS_REG_IPC_RECV`, `SYS_REG_IPC_PEND`, `SYS_REG_IPC_LIST`) sono attive.
- **Da Farsi**: Sostituire le restanti costanti hardcoded nei driver con query dinamiche effettuate sull'albero di registro `/sys/drivers/` popolato durante la fase di boot.

### 🟡 Fase 4: Header Synchronization & Cleaning (In Corso)
- **Cosa è fatto**: È stata avviata la pulizia dei file doppi in `user/sys/include/sys`.
- **Cosa manca**: Il caricatore ELF del kernel (`kernel/core/src/sched/elf.c`) include ancora `elf.h` pescato dal percorso `user/`. Questa è una violazione del confine kernel/userland.
- **Da Farsi**: Separare nettamente gli header. Creare `kernel/core/include/core/elf.h` ad uso esclusivo del kernel, in modo che il kernel non dipenda in alcun modo dagli header userland.

### 🟢 Fase 5: Restore Kernel-Resident VFS e Compositor & Syscalls (COMPLETATO)
- **Cosa è fatto**: Il compositore grafico reale (`compositor.c`) e il caricamento del file system tramite codice a blocchi indiretti di `boot_fs.c` sono residenti nel kernel. Le chiamate di sistema grafiche e di I/O sono completamente instradate. Il sistema si avvia e apre la shell in una finestra del compositore in modo totalmente stabile su entrambe le architetture.
- **Memory Hardening**: Protezione a livello MMU per forzare l'isolamento assoluto delle pagine utente (impedendo la lettura/scrittura di memoria del kernel da user-space) integrata stabilmente.

### 🟢 Fase 6: Multi-Arch Build & Verification (COMPLETATO)
- **Stato AArch64**: Compila perfettamente e si avvia in QEMU, eseguendo con successo `init`, il server di notifiche e la shell interattiva senza alcun crash o eccezione (`make test-release` ➔ 🟢 **SUCCESS**).
- **Stato AMD64**: Compila e si avvia stabilmente sia via boot diretto (`make run`) sia in modalità di rilascio tramite ISO ibrida (`make test-release ARCH=amd64`).
- **Risoluzione stabilità AMD64**:
  1. **Boot Initialization Order**: Abbiamo anticipato la chiamata a `cpu_init()` prima di `arch_platform_early_init()` per garantire che le tabelle dei descrittori (GDT, IDT) e i vettori di eccezione siano registrati molto presto nel ciclo di vita del kernel. Questo ci permette di intercettare e visualizzare nel dettaglio qualsiasi eccezione iniziale (Page Fault, GPF) invece di incorrere in un triple fault con reset silente.
  2. **Robust MBR Fallback for ISO Boot**: Durante l'avvio tramite ISO ibrida (usata in `test-release` su AMD64 via GRUB), l'immagine del blocco caricato non presenta una tabella delle partizioni GPT a causa della generazione ibrida. Abbiamo implementato un meccanismo di fallback robusto in `boot_fs.c`: se la firma GPT non è valida, scansioniamo la tabella delle partizioni MBR cercando una partizione nativa Linux (tipo `0x83`). Se trovata, montiamo la partizione Ext4 di userland con successo.

---

## 3. Stato di Compilazione e Test
- **AArch64**: `make clean && make all ARCH=aarch64` ➔ **🟢 OK** | `make test-release` ➔ **🟢 OK (Boot Stabile e Shell Attiva)**
- **AMD64**: `make clean ARCH=amd64 && make all ARCH=amd64` ➔ **🟢 OK** | `make test-release ARCH=amd64` ➔ **🟢 OK (Boot Stabile e Shell Attiva via MBR Fallback)**
