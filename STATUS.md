# 📑 Stato Finale del Refactoring del Microkernel OS1 (Mainbrain Completo)

Questo documento costituisce il rapporto definitivo sullo stato di avanzamento, l'architettura, la disposizione fisica dei file, il piano delle fasi e i metodi di verifica del **Microkernel OS1**, allineato ai principi open-source **GPLv2** (ispirati a Linux) e ai paradigmi di design di **Plan 9**, **seL4** e **Mach4**.

---

## ⚖️ Licenza & Ispirazioni Architetturali

Il microkernel OS1 è interamente concesso sotto licenza **GNU General Public License, Version 2 (GPLv2)**, garantendo piena comunanza con l'ecosistema open-source globale di **Linux**.

### Mappatura delle Fonti e delle Ispirazioni nel Codebase:
1. **Plan 9 (Bell Labs) [Pilastro Principale]**:
   * *Filosofia*: "Ogni risorsa è un file/nodo gerarchico" ed è gestibile mediante code serializzate.
   * *Riferimento nel Codice*: Implementazione del registro gerarchico dinamico e delle code ad anello per IPC serializzato in [registry.c](file:///Users/olmo/Documents/git/ostest1/os1test-dev/kernel/libkernel/src/registry.c).
2. **seL4 (Secure Embedded L4) [Pilastro Principale]**:
   * *Filosofia*: Hardware Abstraction Layer (HAL) estremamente ridotto (thinned) e confinato alla sola gestione dei vettori assembly e al cambio di contesto.
   * *Riferimento nel Codice*: Vettori assembly e gestione dei registri in [exception.S](file:///Users/olmo/Documents/git/ostest1/os1test-dev/kernel/hal/arch/aarch64/cpu/exception.S) (AArch64) e [start.S](file:///Users/olmo/Documents/git/ostest1/os1test-dev/kernel/hal/arch/amd64/boot/start.S) (AMD64).
3. **Linux (Kernel)**:
   * *Filosofia*: Strutture dati intrinseche ad altissima efficienza (liste concatenate), parser del filesystem a blocchi Ext4 e partizioni GPT.
   * *Riferimento nel Codice*: Liste circolari doppiamente concatenate in [list.h](file:///Users/olmo/Documents/git/ostest1/os1test-dev/kernel/core/include/core/list.h), parser Ext4 in [ext4.c](file:///Users/olmo/Documents/git/ostest1/os1test-dev/kernel/core/src/fs/ext4.c) e GPT in [gpt.c](file:///Users/olmo/Documents/git/ostest1/os1test-dev/kernel/core/src/fs/gpt.c).
4. **Progetto base-nexs**:
   * *Filosofia*: Paradigma di mappatura unificata dei servizi di sistema e protocolli di ciclo del registro.
   * *Riferimento nel Codice*: Logica del registro in [registry.c](file:///Users/olmo/Documents/git/ostest1/os1test-dev/kernel/libkernel/src/registry.c) e coordinazione dei driver.
5. **BSD / FreeBSD**:
   * *Filosofia*: Virtual File System (VFS) a nodi virtuali (vnode) e traduzione dei percorsi (`namei`).
   * *Riferimento nel Codice*: Interfacce vfsops e vnode pre-mappate in [vfs.h](file:///Users/olmo/Documents/git/ostest1/os1test-dev/kernel/core/include/core/vfs.h).
6. **Mach4 (Mach Microkernel)**:
   * *Filosofia*: Processi e server di sistema isolati che comunicano tramite porte IPC e code di messaggi asincrone.
   * *Riferimento nel Codice*: Interfaccia di chiamata di sistema in [syscall.c](file:///Users/olmo/Documents/git/ostest1/os1test-dev/kernel/core/src/syscall.c).
7. **Librerie stb (stb_truetype & stb_easy_font)**:
   * *Filosofia*: Rasterizzazione tipografica leggera e indipendente.
   * *Riferimento nel Codice*: Parsing TTF in [stb_truetype.h](file:///Users/olmo/Documents/git/ostest1/os1test-dev/tools/stb_truetype.h) e rendering font utente in [stb_easy_font.h](file:///Users/olmo/Documents/git/ostest1/os1test-dev/user/sys/include/stb_easy_font.h).
8. **Limine**:
   * *Filosofia*: Protocolli di boot multi-stage e passaggio strutturato dei metadati del bootloader.
   * *Riferimento nel Codice*: Setup assembly in [kernel/hal/boot/](file:///Users/olmo/Documents/git/ostest1/os1test-dev/kernel/hal/boot/).

---

## 🎯 Punto e Stato Attuale (Analisi della Chat)

Siamo giunti al completamento con successo delle fasi cardine di stabilizzazione, relocation e decoupling architetturale del microkernel. 

### 🟢 Fase 1: HAL Decoupling, Relocation, e Thinning (COMPLETATO 100%)
*   **Startup e Bootstrap**: Tutto l'assembly obsoleto e non tracciato è stato rimosso dalla root del progetto e riorganizzato ordinatamente sotto la HAL:
    *   [kernel/hal/boot/aarch64/](file:///Users/olmo/Documents/git/ostest1/os1test-dev/kernel/hal/boot/aarch64/)
    *   [kernel/hal/boot/amd64/](file:///Users/olmo/Documents/git/ostest1/os1test-dev/kernel/hal/boot/amd64/)
    *   [kernel/hal/user/](file:///Users/olmo/Documents/git/ostest1/os1test-dev/kernel/hal/user/)
*   **Pulizia Userland**: Eliminazione fisica e dal tracciamento git del file duplicato ed inutilizzato `user/sys/lib/syscall.S`.
*   **Makefile Hardening**: Il build system è stato interamente irrobustito per compilare in modo indipendente e preciso i moduli per entrambe le architetture target (`ARCH=aarch64` e `ARCH=amd64`).

### 🟢 Fase 2: Driver Decoupling & MMIO / PCI Decoupling (COMPLETATO 100%)
*   **Riorganizzazione Fisica**: I driver hardware sono stati fisicamente separati in base al tipo di bus in sotto-cartelle dedicate:
    *   `drivers/mmio/`: UART Seriali, GIC (ARM), PIC/PIT (X86).
    *   `drivers/pci/`: Storage (VirtIO-Block), Grafica (VirtIO-GPU), Input (Keyboard).
*   **Protocollo a Messaggi**: Rimossa l'inclusione di funzioni C dirette. I driver registrano i propri callback mediante l'interfaccia a messaggi `hw_driver` in [drivers.c](file:///Users/olmo/Documents/git/ostest1/os1test-dev/kernel/core/src/drivers.c) ed eseguono comandi tramite messaggi IPC strutturati (`REG_MSG_BLK_READ`, `REG_MSG_MMIO_WRITE`, ecc.).

### 🟢 Isolamento degli Header & Sicurezza (COMPLETATO 100%)
*   **Eradicata Dipendenza Kernel-Userland (elf.h)**: L'header di parsing ELF del kernel [elf.h](file:///Users/olmo/Documents/git/ostest1/os1test-dev/kernel/core/include/core/elf.h) è stato riscritto come completamente autonomo e isolato in kernel-space.
*   **Eradicata Dipendenza Kernel-Userland (ipc.h, abi.h, font.h, errno.h)**: Creati header kernel-private in [ipc_types.h](file:///Users/olmo/Documents/git/ostest1/os1test-dev/kernel/libkernel/include/libkernel/ipc_types.h), [abi.h](file:///Users/olmo/Documents/git/ostest1/os1test-dev/kernel/core/include/core/abi.h), [font.h](file:///Users/olmo/Documents/git/ostest1/os1test-dev/kernel/core/include/core/font.h), [errno.h](file:///Users/olmo/Documents/git/ostest1/os1test-dev/kernel/core/include/core/errno.h). Rimosso `-Iuser/sys/include` dai CFLAGS del kernel nel Makefile. I tipi condivisi sono definizioni ABI duplicate intenzionalmente (stesso pattern di `elf.h`).
*   **Makefile Namespace Isolation**: Il Makefile utilizza ora `KERNEL_INCLUDE` (senza path userland) per la compilazione kernel e `USER_INCLUDE` (con `-Iuser/sys/include`) per la compilazione userland.

### ⚠️ Compositor Kernel-Resident (Fase 3.5 Pianificata)
*   **Stato**: `Conosciuto / Fase 3.5 Pianificata`
*   **Situazione Attuale**: Il compositor grafico risiede nel kernel-space in [compositor.c](file:///Users/olmo/Documents/git/ostest1/os1test-dev/kernel/core/src/graphics/compositor.c) con 10 syscall dedicate (`SYS_CREATE_WINDOW`, `SYS_WINDOW_BLIT`, `SYS_COMPOSITOR_RENDER`, ecc.) dispatchate direttamente in [syscall.c](file:///Users/olmo/Documents/git/ostest1/os1test-dev/kernel/core/src/syscall.c).
*   **Fase 3.5**: Migrazione del compositor a un demone user-space che comunica con il kernel tramite IPC e buffer condivisi, eliminando le syscall grafiche dirette e completando il modello microkernel.

### 🟡 Fase 3a: VFS Skeleton (Prerequisito)
*   **Stato**: `Pianificato`
*   **Obiettivo**: Costruzione dell'infrastruttura VFS minima: tabella fd per-processo, interfacce `vfsops`/`vnodeops`, mount table, implementazione di `sys_open/read/write/close` attraverso il layer vnode.
*   **Nota**: Le syscall VFS in [stubs.c](file:///Users/olmo/Documents/git/ostest1/os1test-dev/kernel/core/src/stubs.c) attualmente ritornano `-ENOSYS`. [vfs.h](file:///Users/olmo/Documents/git/ostest1/os1test-dev/kernel/core/include/core/vfs.h) contiene un solo prototipo (`vfs_resolve_path`).

### 🟡 Fase 3b: Integrazione Registro-VFS (Bridge)
*   **Stato**: `Pianificato`
*   **Obiettivo**: Montaggio del registro dinamico in `/sys/registry` come pseudo-filesystem nel VFS, permettendo l'I/O verso i driver mediante normali chiamate POSIX `open`/`read`/`write` sui nodi virtualizzati.
*   **Dipende da**: Fase 3a completata.

### 🟡 Fase 3c: Hardware Autodiscovery
*   **Stato**: `Pianificato`
*   **Obiettivo**: Parsing di FDT (AArch64) e Multiboot2 tags (AMD64) dopo `registry_init()` per popolare `hardware/<device>/base_address` e `hardware/<device>/irq`.
*   **Nota AMD64**: Il puntatore Multiboot2 `mbi_ptr` è attualmente scartato con `(void)mbi_ptr` in [main.c](file:///Users/olmo/Documents/git/ostest1/os1test-dev/kernel/core/src/main.c). Richiede implementazione del walker dei tag Multiboot2.

---

## 📂 Disposizione Fisica dei File (Catalogo del Deposito)

```
.
├── LICENSE                          # Licenza GNU GPLv2
├── Makefile                         # Central Build System
├── README.md                        # Documentazione Sviluppatore
├── REFACTOR_PLAN.md                 # Roadmap multi-fase di Refactoring
├── STATUS.md                        # Stato corrente (Questo documento)
├── WORK_SUMMARY.md                  # Log cronologico dello sviluppo
├── grub.cfg                         # Configurazione ISO Grub (x86_64)
│
├── kernel/                          # KERNEL SPACE
│   ├── core/                        # Moduli Indipendenti dall'Architettura
│   │   ├── include/core/            # Header di Sistema (sched.h, vfs.h, drivers.h)
│   │   │   ├── elf.h                # [ISOLATO] Header ELF autonomo del kernel
│   │   │   ├── ipc.h                # [ISOLATO] Forwarder a libkernel/ipc_types.h
│   │   │   ├── abi.h                # [ISOLATO] Numeri syscall kernel-private
│   │   │   ├── font.h               # [ISOLATO] Strutture font kernel-private
│   │   │   └── errno.h              # [ISOLATO] Codici errore POSIX kernel-private
│   │   └── src/                     # Core del kernel (main.c, stubs.c, drivers.c)
│   │       ├── fs/                  # Filesystem residenti (ext4.c, gpt.c)
│   │       └── graphics/            # Compositor grafico residente (compositor.c)
│   │
│   ├── hal/                         # HARDWARE ABSTRACTION LAYER
│   │   ├── arch/                    # File Assembly e CPU-Specifici
│   │   │   ├── aarch64/             # Inizializzazione MMU, Exception vectors
│   │   │   └── amd64/               # GDT, IDT, APIC, start assembly
│   │   ├── boot/                    # Bootloaders & Early stages
│   │   ├── drivers/                 # Driver Fisici Decoupled
│   │   │   ├── mmio/                # Drivers MMIO (uart/pl011.c, uart/16550.c)
│   │   │   └── pci/                 # Drivers PCI (storage/virtio_blk.c)
│   │   └── user/                    # Startup e trampolini syscall dell'Userland
│   │
│   └── libkernel/                   # LIBRERIA CONDIVISA DEL KERNEL
│       ├── include/libkernel/       # Tipi base, stringhe, ctype, math, ipc_types.h
│       └── src/                     # Implementazioni (registry.c, printk.c, kmalloc.c)
│
├── tools/                           # Strumenti di Compilazione Host (mkdisk, etc.)
│
└── user/                            # USER SPACE (EL0)
    ├── bin/                         # Applicazioni utente (demo3d, counter)
    └── sys/                         # Demoni critici di sistema
        ├── bin/                     # init.c, shell.c, regedit.c, fontman
        ├── include/                 # Header standard di userland (stdio.h, unistd.h)
        └── lib/                     # Libc utente (malloc.c, rendering grafico lib.c)
```

---

## 🔬 Metodi di Test e Verifica

La stabilità e la correttezza del codice su entrambe le architetture sono verificate tramite una suite robusta di compilazione e testing in ambiente simulato QEMU.

### 1. Verifica dell'Integrità dei Toolchain
Verifica la corretta rilevazione e configurazione dei compilatori incrociati e degli emulatori:
```bash
make check ARCH=aarch64
make check ARCH=amd64
```
*Output Atteso:* Rilevamento positivo di `gcc`, `as`, `ld`, `qemu` e `grub-mkrescue` per il target selezionato.

### 2. Test di Compilazione Pulita
Esegue la pulizia e la ricostruzione dell'intero albero di sistema (Kernel, HAL, Librerie, Applicazioni Userland e generazione dell'immagine Ext4 del disco):
```bash
# Compilazione completa AArch64
make clean ARCH=aarch64 && make all ARCH=aarch64

# Compilazione completa AMD64
make clean ARCH=amd64 && make all ARCH=amd64
```
*Output Atteso:* Creazione delle immagini disco `build/aarch64/disk.img` (raw binary) e `build/amd64/disk.img` (elf).

### 3. Test di Esecuzione e Release in QEMU
Verifica l'avvio del sistema, il corretto mounting GPT/Ext4 dell'immagine disco, l'attivazione della shell e dei demoni grafici:
```bash
# Avvio di test per AArch64
make test-release ARCH=aarch64

# Avvio di test per AMD64 (con Grub e fallback MBR)
make test-release ARCH=amd64
```
*Comportamento Atteso:* Avvio di QEMU con caricamento del kernel OS1, visualizzazione del banner di avvio, caricamento dei driver tramite messaggi, caricamento ed esecuzione di `/sys/bin/init` (PID 1) che avvia il compositor ed apre la shell interattiva dell'utente.

---

## 🧠 Conoscenze Accumulate e Best Practices

1. **Allocazione del Registro con Pool Statico + Code Heap-Bounded**:
   * I nodi del registro gerarchico in [registry.c](file:///Users/olmo/Documents/git/ostest1/os1test-dev/kernel/libkernel/src/registry.c) sono allocati da un pool statico (`REG_POOL_SIZE = 256`), evitando frammentazione dell'heap per la struttura ad albero. Le code IPC (`struct reg_queue`, 386 byte ciascuna) sono allocate via `kmalloc` al momento della registrazione dei driver (bounded early-boot heap allocation).
2. **Uso delle Code ad Anello per IPC**:
   * Ogni nodo di coda del registro dispone di un ring-buffer protetto da spinlock locali che gestisce la ricezione/invio asincrono di messaggi `reg_msg` (profondità coda: `REG_QUEUE_DEPTH = 16`). Le code sono heap-allocate al tempo di registrazione del driver tramite `reg_ipc_init_queue()`.
3. **Isolamento della HAL**:
   * La HAL deve rimanere unicamente lo strato hardware primario di aggancio per i registri CPU. Qualsiasi calcolo logico sui descrittori di processo, tabelle VFS o blitting grafici risiede fermamente nel Kernel Core unificato, preservando la portabilità.
4. **Namespace Sanitization**:
   * I file in `kernel/` non devono mai importare header esterni a `kernel/` o `libkernel/`. Eventuali strutture condivise (come le definizioni degli eseguibili in `elf.h`) devono essere duplicate ed isolate in kernel-space per prevenire namespace leakage e build contaminati.
