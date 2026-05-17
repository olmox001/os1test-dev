# Critica e Piano Rivisto: HAL Unificata, GPU Abstraction & Servizi Privilegiati

Questo documento analizza lo stato corrente del codebase rispetto al piano proposto
("Architettura HAL Unificata"), identifica le lacune e gli errori nel piano stesso,
e produce una versione rivista con le dipendenze corrette.

---

## Sezione 1 — Stato Corrente Verificato

Prima di criticare il piano, ecco cosa il codebase già contiene o ha già risolto.

### Fix già applicati (plan_fix.md)

| Item | Stato | Verifica |
|:-----|:------|:---------|
| A1 — ipc.h leakage | **COMPLETATO** | `core/ipc.h` include `<libkernel/ipc_types.h>`; Makefile usa `KERNEL_INCLUDE` senza `user/sys/include` |
| A2 — Heap claim in STATUS.md | **NON AGGIORNATO** | `registry.c:212` chiama ancora `kmalloc(sizeof(struct reg_queue))` — la documentazione non è corretta |
| A3 — REG_POOL_SIZE 128 vs 256 | **NON AGGIORNATO** | `registry.h:9` definisce `256`; STATUS.md dice ancora 128 |
| A4 — Phase 4 Step 1 elf.h | **COMPLETATO** | `kernel/core/include/core/elf.h` è autonomo e isolato |
| A5 — Compositor kernel-resident | **RICONOSCIUTO** | Il nuovo piano lo pianifica come Phase 5c — il riconoscimento è corretto |

### Infrastruttura già presente rilevante per il piano proposto

Questi file esistono e funzionano: il piano proposto spesso non li cita, creando
il rischio di duplicazione.

- **`kernel/hal/src/bus.c`** — già implementa `hal_bus_init()`, `hal_register_device()`,
  `hal_device_find()`, `hal_device_get()`. Il bus manager è operativo.
- **`kernel/hal/include/hal/drivers/gpu/gpu.h`** — già definisce `struct gpu_ops`,
  `struct gpu_device`, `gpu_register()`, `gpu_unregister()`, `gpu_get_primary()`.
- **`kernel/hal/drivers/pci/graphics/gpu_core.c`** — implementa il registro GPU con spinlock.
- **`kernel/hal/drivers/pci/graphics/virtio_gpu.c`** — VirtIO-GPU completo con flush,
  transfer, resource creation (attualmente interni a `struct virtio_gpu_state`).
- **`kernel/core/include/core/hal_device.h`** — `hal_device_t` con `hal_dev_read32/write32`
  per accesso MMIO e Port I/O unificato.
- **`kernel/core/include/core/virtio_hal.h`** — `struct virtio_hw_ops` + `struct virtio_device`
  già definiti.
- **`kernel/core/include/core/drivers.h`** — `struct hw_driver` con `dispatch` callback
  message-based usato da UART e VirtIO-Block.

### Stato fasi precedenti

```
Phase 1  HAL Decoupling          [x] COMPLETATO
Phase 2  Driver Message-Based    [x] COMPLETATO
Phase 3a VFS Skeleton            [ ] NON INIZIATO
Phase 3b Registry-VFS Bridge     [ ] NON INIZIATO (blocca Phase 5b BSD VFS Module)
Phase 3c Hardware Autodiscovery  [ ] NON INIZIATO
Phase 4  Header Consolidation    [~] PARZIALE (ipc.h + elf.h OK; audit finale pending)
Phase 5  HAL Unificata (nuovo)   [ ] PROPOSTO — sotto critica in questo documento
```

---

## Sezione 2 — Critica al Piano Proposto

### Tier A — Falsi Presupposti Architetturali

Questi difetti porterebbero a costruire su basi errate o a duplicare infrastruttura esistente.

---

#### A1. Il piano crea infrastruttura parallela al bus manager esistente

**Affermazione del piano**: "Creare `kernel/hal/bus/include/hal/bus/bus.h` con
`struct bus_ops`, `struct bus_device`, `bus_register_device()`, `bus_find_device()`."

**Realtà**: `kernel/hal/src/bus.c` già implementa:
```c
void hal_bus_init(void);
void hal_register_device(struct hal_device *dev);
struct hal_device *hal_device_find(uint16_t vendor, uint16_t device, int index);
struct hal_device *hal_device_get(int index);
```
E `hal_device.h` già ha le primitive di accesso MMIO/Port I/O (`hal_dev_read32` ecc.).
Creare una nuova directory `hal/bus/` con funzionalità parallele produce duplicazione,
non astrazione.

**Fix richiesto**: Estendere `hal/src/bus.c` e `hal_device.h` per aggiungere
`struct bus_ops` come campo opzionale di `hal_device`. Non creare una nuova gerarchia.
Il rename della directory è cosmesi — non vale il rischio di rottura.

---

#### A2. Il piano propone `gpu_hal.h` come nuovo file quando `gpu.h` esiste già

**Affermazione del piano**: "Creare `kernel/hal/gpu/include/hal/gpu/gpu_hal.h`" e
"DELETE `kernel/hal/drivers/pci/graphics/`".

**Realtà**: `kernel/hal/include/hal/drivers/gpu/gpu.h` esiste già con
`struct gpu_ops` e `struct gpu_device`. Il piano propone di eliminare la directory
`pci/graphics/` dove risiedono `gpu_core.c` e `virtio_gpu.c` — file funzionanti e
già in uso.

**Cosa il piano fa bene**: Le nuove operazioni (`create_resource`, `destroy_resource`,
`map_resource`, `transfer`, `scanout`) sono un'estensione architetturalmente corretta —
espongono al layer HAL operazioni oggi interne a `virtio_gpu_state`. Questo è il lavoro
utile del piano.

**Fix richiesto**: Invece di "DELETE + CREATE", fare:
1. Aggiungere le nuove ops a `gpu.h` esistente (`create_resource`, `transfer`, `scanout`).
2. Spostare `gpu_core.c` da `pci/graphics/` a `hal/gpu/core/` (rename di path, non riscrittura).
3. Aggiornare `virtio_gpu.c` per popolare le nuove ops esponendole dalla struttura privata.
Il DELETE di `pci/graphics/` può avvenire solo a migrazione completata, non come primo passo.

---

#### A3. Phase 5b (BSD VFS Module) dipende da Phase 3a che non è iniziata

**Affermazione del piano**: Phase 5b introduce il "BSD VFS Module" come modulo kernel
privilegiato con syscall `PROC_CAP_VFS`.

**Realtà**: `kernel/core/include/core/vfs.h` contiene **un solo prototipo**:
`vfs_resolve_path()`. Non esistono `struct vfsops`, `struct vnodeops`, fd table
per-processo, mount table. Le syscall VFS in `stubs.c` ritornano `-ENOSYS`.

Un modulo kernel che espone VFS ai servizi privilegiati non può esistere senza
il VFS sottostante. Phase 3a deve completarsi prima che Phase 5b inizi.
Il piano non elenca questa dipendenza.

**Fix richiesto**: Aggiungere esplicitamente nella dependency chain:
```
Phase 3a (VFS Skeleton) → Phase 5b (BSD VFS Module)
```
Phase 5b non può essere pianificata come parallela a Phase 5a.

---

#### A4. La relazione tra `hw_driver` e `driver_node` non è definita

**Affermazione del piano**: "Reimplementare `drivers.c` usando `driver_tree_register`
internamente, mantenendo backward compat con `driver_register()`."

**Realtà**: `struct hw_driver` (in `drivers.h`) e `struct driver_node` (proposto in
`driver_tree.h`) hanno semantiche diverse:
- `hw_driver`: nome + init + dispatch per messaggio. Usato attualmente da PL011, 16550, VirtIO-Block.
- `driver_node`: classe + bus_device + dispatch + registry_entry + ops tipizzati per classe.

Il piano non risponde: `hw_driver` viene deprecated? Wrappato da un adaptor interno
a `driver_node`? Mantenuto come API pubblica con `driver_node` come implementazione?
Senza questa risposta, "backward compat" è una promessa senza contenuto.

**Fix richiesto**: Specificare la transizione:
```
struct driver_node → astrazione principale (nuova)
struct hw_driver   → deprecated, wrappato da un adaptor durante la transizione
driver_register()  → rimane come shim che chiama driver_tree_register() internamente
```
Deadline di rimozione dello shim: Phase 5c completata.

---

### Tier B — Lacune di Progettazione

---

#### B1. Le Open Questions del piano non sono chiuse

Il piano marca due domande come "IMPORTANT open questions" ma non le risolve mai
nella sezione tecnica:

**Domanda 1**: "Il compositor accede al framebuffer via SYS_GPU_MAP_FB o mappatura
diretta kernel-side?"

Più avanti nel piano compare `SYS_COMP_MAP_FB = 290` senza collegarlo alla domanda.
Il nome cambia da `GPU_MAP_FB` a `COMP_MAP_FB` senza spiegazione. Questo genera
ambiguità su quale syscall venga implementata.

**Risposta da fissare**: La syscall è `SYS_COMP_MAP_FB` (290), riservata al compositor
(richiede `PROC_CAP_COMPOSITOR`). Mappa il framebuffer fisico GPU nell'address space
del processo compositor. Nessun `SYS_GPU_MAP_FB` separato.

**Domanda 2**: "VirtIO-GPU Transport Split: MMIO e PCI in Stage 1 o solo MMIO?"

**Risposta da fissare**: Phase 5a usa solo il trasporto MMIO esistente. Il trasporto
PCI è `[FUTURO]` senza numero di fase. Chiudere la domanda nel piano, non lasciarla aperta.

---

#### B2. Il meccanismo di enforcement dei permessi non è specificato

Il piano aggiunge `PROC_CAP_GPU`, `PROC_CAP_COMPOSITOR`, `PROC_CAP_VFS` a `sched.h`
ma non specifica **dove** vengono controllati nel kernel.

Senza questa specifica, i bit esistono ma non sono applicati. Le syscall privilegiate
possono essere invocate da qualsiasi processo.

**Fix richiesto**: Aggiungere una sezione "Permission Enforcement" con:
```c
/* In syscall.c, prima del dispatch delle syscall privilegiate */
static int cap_check(struct process *p, uint32_t required_cap) {
    if (!(p->permissions & required_cap)) return -EPERM;
    return 0;
}
```
E specificare dove nel dispatch chain di `syscall.c` viene invocato (prima o dopo
il switch-case? Come helper inline?).

---

#### B3. Nessun piano di transizione per le 10 syscall compositor esistenti

Il piano rimuove SYS_CREATE_WINDOW (210), SYS_WINDOW_BLIT (213), SYS_COMPOSITOR_RENDER (212)
e altre 7 syscall compositor. Queste sono usate da processi utente esistenti: shell,
demo3d, notifiche.

Se le syscall vengono rimosse prima che le app siano migrate a IPC compositor, il sistema
non avvia. Il piano non menziona questo.

**Fix richiesto**: Aggiungere una fase di transizione esplicita:
1. Phase 5c.1: Il compositor daemon viene avviato, le vecchie syscall rimangono attive
   come shim (chiamano il compositor via IPC internamente al kernel).
2. Phase 5c.2: Le app utente vengono migrate a IPC compositor.
3. Phase 5c.3: Le vecchie syscall vengono rimosse da `abi.h` e `syscall.c`.
Nessun "clean cut" senza la garanzia che tutte le app siano già migrate.

---

#### B4. Il gap nei numeri syscall non è documentato

`abi.h` arriva a SYS_REG_LIST = 263. Il piano propone SYS_CL_* a 280-286 e
SYS_COMP_* a 290-292. Il range 264-279 non è spiegato.

**Fix richiesto**: Aggiungere a `abi.h` una sezione commentata che documenta la policy
di allocazione dei numeri syscall:
```
200-215  Grafica kernel (legacy, deprecate in Phase 5c)
216-229  Processi e memoria
230-249  IPC
250-265  Filesystem e Registry
266-279  RISERVATO per future estensioni Phase 3/4
280-289  kGPU Resource Module (Phase 5b)
290-299  Compositor Primitives (Phase 5b)
```

---

#### B5. Il nome "OpenCL" crea aspettative incompatibili con l'implementazione

Il piano chiama il modulo "OpenCL Kernel Module" e le syscall `SYS_CL_*`. OpenCL è
una specifica KHRONOS con un compilatore, un runtime, un modello di memoria e API
definiti — nessuno dei quali viene implementato qui.

Quello che il piano descrive è un'API di gestione risorse GPU kernel-side con un
modello buffer/queue ispirato a OpenCL. È un'ispirazione, non un'implementazione.

**Fix richiesto**: Rinominare in tutta la documentazione e nel codice:
- "OpenCL Kernel Module" → "kGPU Resource Module"
- `SYS_CL_*` → `SYS_KGPU_*` (o mantenere `SYS_CL_*` come abbrev. documentata come "kernel GPU")
- L'ispirazione OpenCL può essere menzionata in un commento di testa, non nel nome del modulo

---

### Tier C — Errori Documentali Residui

| Location | Errore | Fix |
|:---------|:-------|:----|
| STATUS.md, Best Practices #1 | `REG_POOL_SIZE = 128` | Aggiornare a 256 |
| STATUS.md, Best Practices #1 | "assoluta assenza di heap fragmentation" | Aggiungere: "le IPC queue usano kmalloc bounded al boot" |
| REFACTOR_PLAN.md, Phase 4 Step 1 | Ancora aperto | Marcare `[x] DONE` |
| Piano nuovo, tabella dimensioni | `compositor.c` = 35KB | File è 1172 righe, ~35KB OK ma va verificato |
| Piano nuovo, `graphics.c` | Eliminato senza verifica dipendenze | Verificare `grep -r "graphics.h\|graphics_init" kernel/` prima |

---

## Sezione 3 — Piano Rivisto

### Decisioni architetturali confermate (immutate)

- Il compositor migra in userspace come servizio privilegiato.
- Nessun TTY kernel — i terminali vivono nel compositor.
- Il Message Bus HAL resta nel kernel.
- Le app normali comunicano col compositor via IPC Plan 9-style.
- Il VFS daemon accede ai primitivi VFS via syscall privilegiata.

### Decisioni aperte ora chiuse

| Domanda | Decisione |
|:--------|:----------|
| Framebuffer mapping | `SYS_COMP_MAP_FB` (290): syscall privilegiata, richiede `PROC_CAP_COMPOSITOR` |
| VirtIO-GPU Transport | Phase 5a: solo MMIO. PCI transport: fase futura non numerata, non bloccante |
| `hw_driver` vs `driver_node` | `driver_node` è l'astrazione principale; `hw_driver` wrappato da adaptor in transizione |
| Nome modulo GPU kernel | "kGPU Resource Module" — non "OpenCL" |

---

### DAG delle Dipendenze (Corretto)

```
Phase 3a (VFS Skeleton)
    ↓
Phase 3b (Registry-VFS Bridge) ←→ Phase 3c (Hardware Autodiscovery) [parallele dopo 3a]
    ↓
Phase 4 (Header Consolidation — audit finale)
    ↓
Phase 5a (Bus HAL extension + Driver Tree + GPU HAL extension) ← PRIORITÀ ASSOLUTA
    ↓
Phase 5b (kGPU Module + Compositor Primitives + BSD VFS Module*)
    |   * richiede Phase 3a
    ↓
Phase 5c (Servizi Usermode: Compositor daemon, VFS daemon, Mesa)
    ↓
Phase 5c.cleanup (Rimozione vecchie syscall compositor dopo migrazione app)
```

---

### Fase 5a — Bus HAL Extension + Driver Tree + GPU HAL (Rivista)

#### 5a.1 — Estendere il bus manager esistente (NON ricreare)

**File da modificare**: `kernel/hal/src/bus.c` e `kernel/hal/include/hal/drivers/virtio.h`

Aggiungere a `hal_device` il campo opzionale `struct bus_ops *ops`:
```c
/* Aggiunta a struct hal_device in hal.h */
struct bus_ops {
    uint32_t (*read32)(void *handle, uint32_t reg);
    void     (*write32)(void *handle, uint32_t reg, uint32_t val);
    uint16_t (*read16)(void *handle, uint32_t reg);
    void     (*write16)(void *handle, uint32_t reg, uint16_t val);
    uint8_t  (*read8)(void *handle, uint32_t reg);
    void     (*write8)(void *handle, uint32_t reg, uint8_t val);
    int      (*setup_queue)(void *handle, uint32_t idx,
                            uint64_t desc, uint64_t avail, uint64_t used);
    void     (*notify)(void *handle, uint32_t queue);
};
```
I trasporti MMIO e PCI implementano questa interfaccia. Non serve una nuova directory.

#### 5a.2 — Driver Tree Node

**File nuovo**: `kernel/core/include/core/driver_tree.h`
**File modificato**: `kernel/core/src/drivers.c` — reimplementare internamente con driver_node, esporre `driver_register()` come shim per backward compat

Specifica della transizione `hw_driver` → `driver_node`:
```c
/* adaptor interno in drivers.c — non esposto pubblicamente */
static int hw_driver_adaptor_dispatch(struct driver_node *node,
                                      const struct reg_msg *msg,
                                      struct reg_msg *reply) {
    struct hw_driver *drv = (struct hw_driver *)node->ops;
    return drv->dispatch(msg, reply);
}

int driver_register(struct hw_driver *drv) {
    struct driver_node *node = /* alloc da pool statico */;
    node->ops = drv;
    node->dispatch = hw_driver_adaptor_dispatch;
    return driver_tree_register(node);
}
```

#### 5a.3 — Estendere GPU HAL (NON ricreare)

**File modificato**: `kernel/hal/include/hal/drivers/gpu/gpu.h` — aggiungere:
```c
struct gpu_ops {
    /* Esistenti — invariati */
    int  (*init)(struct gpu_device *dev);
    int  (*set_mode)(struct gpu_device *dev, int w, int h);
    void *(*get_framebuffer)(struct gpu_device *dev, size_t *size);
    int  (*flush)(struct gpu_device *dev, int x, int y, int w, int h);
    void (*destroy)(struct gpu_device *dev);

    /* NUOVE — kGPU Resource Model */
    int   (*create_resource)(struct gpu_device *dev, uint32_t w, uint32_t h,
                             uint32_t fmt, uint32_t *id_out);
    int   (*destroy_resource)(struct gpu_device *dev, uint32_t id);
    void *(*map_resource)(struct gpu_device *dev, uint32_t id, size_t *size_out);
    int   (*transfer)(struct gpu_device *dev, uint32_t id,
                     int x, int y, int w, int h);
    int   (*scanout)(struct gpu_device *dev, uint32_t id, int display_id);

    const char *transport_name; /* "mmio" | "pci" */
};
```

**File modificato**: `kernel/hal/drivers/pci/graphics/virtio_gpu.c` — esporre le operazioni
resource management (già implementate internamente) tramite le nuove ops.

**File spostato (non riscritto)**: `gpu_core.c` → `kernel/hal/gpu/core/gpu_core.c`.
La directory `pci/graphics/` viene eliminata solo a spostamento completato.

---

### Fase 5b — Moduli Kernel (kGPU, Compositor Primitives, BSD VFS)

#### Prerequisiti verificati prima di iniziare:
- [ ] Phase 3a (VFS Skeleton) completata
- [ ] Phase 5a completata
- [ ] `driver_node` in produzione per tutti i driver esistenti

#### Sistema Permessi

**File modificato**: `kernel/core/include/core/sched.h` — aggiungere dopo i flag esistenti:
```c
/* Nuovi: Service Capabilities */
#define PROC_PERM_SERVICE     (1 << 3)   /* Servizio di sistema privilegiato */
#define PROC_CAP_GPU          (1 << 8)   /* Accesso kGPU Resource Module */
#define PROC_CAP_VFS          (1 << 9)   /* Accesso BSD VFS Module */
#define PROC_CAP_COMPOSITOR   (1 << 10)  /* Accesso esclusivo GPU framebuffer primitives */
#define PROC_CAP_REGISTRY     (1 << 11)  /* Write al driver registry */
#define PROC_CAP_IPC_PRIV     (1 << 12)  /* IPC kernel-bypass */
```

**File modificato**: `kernel/core/src/syscall.c` — aggiungere helper e usarlo prima
del dispatch di ogni syscall privilegiata:
```c
static int cap_check(uint32_t required) {
    struct process *p = get_current_process();
    if (!p || !(p->permissions & required)) return -EPERM;
    return 0;
}

/* Esempio nel dispatch: */
case SYS_KGPU_CREATE_BUFFER:
    if (cap_check(PROC_CAP_GPU) < 0) { regs->x0 = -EPERM; break; }
    /* ... */
```

#### kGPU Resource Module

**File nuovo**: `kernel/core/src/modules/kgpu.c`

Syscall (range 280-289 in `abi.h`):
```c
#define SYS_KGPU_CREATE_BUFFER  280
#define SYS_KGPU_DESTROY_BUFFER 281
#define SYS_KGPU_MAP_BUFFER     282
#define SYS_KGPU_TRANSFER       283
#define SYS_KGPU_FLUSH          284
#define SYS_KGPU_SCANOUT        285
#define SYS_KGPU_SET_MODE       286
```

Ogni handler verifica `PROC_CAP_GPU`, poi chiama `gpu_get_primary()->ops->*`.

#### Compositor Primitives Module

**File nuovo**: `kernel/core/src/modules/compositor_prim.c`

Syscall (range 290-299 in `abi.h`):
```c
#define SYS_COMP_MAP_FB         290   /* Risposta alla Open Question #1 */
#define SYS_COMP_DAMAGE_NOTIFY  291
#define SYS_COMP_VSYNC_WAIT     292
```

Ogni handler verifica `PROC_CAP_COMPOSITOR`.

#### BSD VFS Module

Dipende da Phase 3a. Non iniziare prima che `struct vfsops`, `struct vnodeops`,
fd table per-processo e mount table esistano nel codice.

**Quando Phase 3a è completa**: Aggiungere syscall `PROC_CAP_VFS` che espongono
le operazioni vnode ai servizi privilegiati. I numeri saranno nel range 300-309
(documentare in `abi.h`).

---

### Fase 5c — Servizi Usermode (Rivista)

#### Struttura

```
user/sys/bin/compositor/
├── compositor.c    # Window manager, z-ordering, damage tracking
├── terminal.c      # Emulatore terminale ANSI
└── input.c         # Dispatch eventi mouse/keyboard

user/lib/libgl/
├── gl.c            # Surface primitives
├── draw2d.c        # 2D shapes
├── draw3d.c        # Software 3D renderer
├── font.c          # Font rendering
└── region.c        # Region algebra
```

**File kernel eliminati solo dopo migrazione completa**:
- `kernel/core/src/graphics/compositor.c`
- `kernel/core/src/graphics/gl.c`
- `kernel/core/src/graphics/draw2d.c`
- `kernel/core/src/graphics/draw3d.c`
- `kernel/core/src/graphics/font.c`
- `kernel/core/src/graphics/region.c`
- `kernel/core/src/graphics/graphics.c` (verificare dipendenze prima con `grep -r`)

#### Piano di Transizione Syscall Compositor (Obbligatorio)

**Phase 5c.1** — Compositor daemon avviato. Le vecchie syscall (210-215) rimangono attive
come shim nel kernel: ricevono la richiesta, la inoltrano via IPC al compositor daemon,
ritornano il risultato. Zero regressioni.

**Phase 5c.2** — Le app utente vengono migrate a IPC compositor una per una. Ogni app
migrata viene verificata con `make test-release` prima di procedere.

**Phase 5c.3** — Quando tutte le app sono migrate: rimuovere gli shim da `syscall.c`
e i numeri da `abi.h`. Aggiungere `#error` nei numeri rimossi per catch a compile time.

---

### Protocollo IPC Compositor

```c
#define COMP_MSG_CREATE_WIN    0x01
#define COMP_MSG_DESTROY_WIN   0x02
#define COMP_MSG_BLIT          0x03
#define COMP_MSG_RENDER        0x04
#define COMP_MSG_SET_FLAGS     0x05
#define COMP_MSG_WRITE_TEXT    0x06
#define COMP_MSG_INPUT_EVENT   0x10
```

---

### Verifica per Ogni Fase

#### Phase 5a
```bash
make clean ARCH=aarch64 && make all ARCH=aarch64
make clean ARCH=amd64   && make all ARCH=amd64
make test-release ARCH=aarch64   # regressione zero su display
```
Criteri aggiuntivi:
- Tutti i driver (UART, VirtIO-Block, VirtIO-GPU) registrati via `driver_tree_register()`
- GPU espone `create_resource` + `scanout` nelle ops
- `driver_register()` shim funziona senza modifiche ai driver esistenti

#### Phase 5b
```bash
# Test permessi: processo USER che chiama SYS_KGPU_CREATE_BUFFER → -EPERM
# Test funzionale: SYS_KGPU_FLUSH produce output su display
make test-release ARCH=aarch64
```

#### Phase 5c
```bash
# Boot completo con compositor daemon in userspace
# Shell riceve window via IPC (non syscall diretta)
# Old syscall 210 restituisce -ENOSYS dopo rimozione shim
make test-release ARCH=aarch64
make test-release ARCH=amd64
```

---

### Riepilogo Modifiche al Piano Originale

| Punto | Piano originale | Piano rivisto |
|:------|:----------------|:--------------|
| Bus layer | Nuova dir `hal/bus/` da zero | Estendere `bus.c` esistente con `struct bus_ops` |
| GPU HAL | Nuovo `gpu_hal.h`, delete `pci/graphics/` | Estendere `gpu.h` esistente, spostare file |
| Nome modulo GPU | "OpenCL Kernel Module" | "kGPU Resource Module" |
| Syscall GPU | `SYS_CL_*` | `SYS_KGPU_*` |
| Phase 5b VFS | Implicito senza prerequisiti | Dipende esplicitamente da Phase 3a |
| `hw_driver` transition | "backward compat" vaga | Adaptor shim specificato |
| Old compositor syscalls | Rimosse senza piano | Transizione in 3 step (5c.1→5c.3) |
| Permission enforcement | Bits definiti, check non specificato | `cap_check()` in `syscall.c` specificato |
| Open Question framebuffer | Non chiusa | Risposta: `SYS_COMP_MAP_FB` (290) |
| Open Question transport split | Non chiusa | Risposta: solo MMIO in Phase 5a |
