/*
 * kernel/drivers/keyboard/keyboard.c
 * Keyboard Input Subsystem
 *
 * Translates scancodes to ASCII and provides buffered input
 */
#include <drivers/keyboard.h>
#include <drivers/ps2.h>
#include <drivers/virtio_input.h>
#include <kernel/printk.h>
#include <kernel/sched.h>
#include <kernel/string.h>
#include <kernel/types.h>
#include <posix_types.h>

/* Keyboard state */
static int shift_pressed = 0;
static int ctrl_pressed = 0;
static int caps_lock = 0;

typedef struct {
  const char *name;
  const char *ascii_map;
  const char *shifted_map;
  struct {
    uint16_t code;
    int shifted;
    const char *utf8;
  } utf8_overrides[16];
} keyboard_layout_t;

static const keyboard_layout_t layout_us = {
    .name = "us",
    /* uses standard tables below */
};

static const keyboard_layout_t layout_it = {
    .name = "it",
    .utf8_overrides = {{40, 0, "\xC3\xA0"}, // à
                       {40, 1, "\xC3\x80"}, // À
                       {26, 0, "\xC3\xA8"}, // è
                       {26, 1, "\xC3\xA9"}, // é
                       {39, 0, "\xC3\xB2"}, // ò
                       {41, 0, "\xC3\xB9"}, // ù
                       {43, 0, "\xC3\xAC"}, // ì
                       {0, 0, NULL}}};

static const keyboard_layout_t *current_layout = &layout_us;

/* Scancode to ASCII table (US layout) */
static const char scancode_to_ascii[128] = {
    0,    0,   '1', '2',  '3',  '4', '5',  '6',  /* 0-7 */
    '7',  '8', '9', '0',  '-',  '=', '\b', '\t', /* 8-15 */
    'q',  'w', 'e', 'r',  't',  'y', 'u',  'i',  /* 16-23 */
    'o',  'p', '[', ']',  '\n', 0,   'a',  's',  /* 24-31 */
    'd',  'f', 'g', 'h',  'j',  'k', 'l',  ';',  /* 32-39 */
    '\'', '`', 0,   '\\', 'z',  'x', 'c',  'v',  /* 40-47 */
    'b',  'n', 'm', ',',  '.',  '/', 0,    '*',  /* 48-55 */
    0,    ' ', 0,   0,    0,    0,   0,    0,    /* 56-63 (space at 57) */
    0,    0,   0,   0,    0,    0,   0,    0,    /* 64-71 */
    0,    0,   0,   0,    0,    0,   0,    0,    /* 72-79 */
    0,    0,   0,   0,    0,    0,   0,    0,    /* 80-87 */
    0,    0,   0,   0,    0,    0,   0,    0,    /* 88-95 */
    0,    0,   0,   0,    0,    0,   0,    0,    /* 96-103 */
    0,    0,   0,   0,    0,    0,   0,    0,    /* 104-111 */
    0,    0,   0,   0,    0,    0,   0,    0,    /* 112-119 */
    0,    0,   0,   0,    0,    0,   0,    0     /* 120-127 */
};

/* Shifted scancode to ASCII */
static const char scancode_to_ascii_shift[128] = {
    0,   0,   '!', '@', '#',  '$', '%',  '^',  /* 0-7 */
    '&', '*', '(', ')', '_',  '+', '\b', '\t', /* 8-15 */
    'Q', 'W', 'E', 'R', 'T',  'Y', 'U',  'I',  /* 16-23 */
    'O', 'P', '{', '}', '\n', 0,   'A',  'S',  /* 24-31 */
    'D', 'F', 'G', 'H', 'J',  'K', 'L',  ':',  /* 32-39 */
    '"', '~', 0,   '|', 'Z',  'X', 'C',  'V',  /* 40-47 */
    'B', 'N', 'M', '<', '>',  '?', 0,    '*',  /* 48-55 */
    0,   ' ', 0,   0,   0,    0,   0,    0,    /* 56-63 */
    0,   0,   0,   0,   0,    0,   0,    0,    /* 64-71 */
    0,   0,   0,   0,   0,    0,   0,    0,    /* 72-79 */
    0,   0,   0,   0,   0,    0,   0,    0,    /* 80-87 */
    0,   0,   0,   0,   0,    0,   0,    0,    /* 88-95 */
    0,   0,   0,   0,   0,    0,   0,    0,    /* 96-103 */
    0,   0,   0,   0,   0,    0,   0,    0,    /* 104-111 */
    0,   0,   0,   0,   0,    0,   0,    0,    /* 112-119 */
    0,   0,   0,   0,   0,    0,   0,    0     /* 120-127 */
};

/*
 * Initialize keyboard subsystem
 */
void keyboard_init(void) {
  shift_pressed = 0;
  ctrl_pressed = 0;
  caps_lock = 0;

  pr_info("%s", "Input: Initializing input subsystem...\n");

  virtio_input_init();

  /* Rilevamento automatico + supporto architetture */
#ifdef ARCH_AMD64
  if (virtio_input_get_dev_count() == 0) {
    pr_info("Input: No VirtIO input → enabling PS/2 fallback\n");
    ps2_init();
  } else {
    pr_info("Input: VirtIO detected (%d) + PS/2 support\n",
            virtio_input_get_dev_count());
    ps2_init(); // Ibrido: meglio avere entrambi su QEMU amd64
  }
#else
  pr_info("%s", "Input: VirtIO only (AArch64 - PS/2 not available)\n");
#endif

  current_layout = &layout_it;

  INIT_LIST_HEAD(&keyboard_wait_queue.task_list);
  spin_lock_init(&keyboard_wait_queue.lock);

  pr_info("Keyboard: Initialized (Layout: %s)\n", current_layout->name);
}

/* Wait Queue for blocking reads */
struct wait_queue_head keyboard_wait_queue;

/*
 * Notification from low-level driver
 * Called from Interrupt Context (VirtIO Handler)
 */
void keyboard_notify_input(void) {
  /* Poll hardware to transfer from VirtIO buffer to Keyboard buffer */
  keyboard_poll();

  /* Wake up waiting processes */
  wake_up(&keyboard_wait_queue);
}

/*
 * Process a key event
 */
static void keyboard_process_key(uint16_t code, int32_t value) {
  /* value: 0 = released, 1 = pressed, 2 = repeat */

  /* Handle modifier keys */
  if (code == KEY_LEFTSHIFT || code == KEY_RIGHTSHIFT) {
    shift_pressed = (value != 0);
    return;
  }

  if (code == KEY_LEFTCTRL) {
    ctrl_pressed = (value != 0);
    return;
  }

  if (code == KEY_CAPSLOCK && value == 1) {
    caps_lock = !caps_lock;
    return;
  }

  /* Handle Ctrl+C: deliver ETX (0x03) to the focused process through the
   * SAME IPC path as normal keys, so read(0)/try_recv see it (the old
   * kb_buffer path was never drained by the fd-based read).  The shell turns
   * a foreground job's Ctrl+C into a kill (USR-TTY-01 #123). */
  if (ctrl_pressed && code == KEY_C && value != 0) {
    if (keyboard_focus_pid > 0) {
      struct ipc_message msg;
      memset(&msg, 0, sizeof(msg));
      msg.from = 0; /* Kernel/Driver */
      msg.type = IPC_TYPE_INPUT;
      msg.data1 = ((uint64_t)code << 16) | 0x03;
      msg.data2 = 1; /* press */
      msg.payload[0] = 0x03;
      msg.payload[1] = '\0';
      kernel_ipc_send(keyboard_focus_pid, &msg);
    }
    return;
  }

  /* Convert scancode to ASCII */
  if (code >= 128)
    return;

  char c;
  int use_shift = shift_pressed;

  /* Apply caps lock to letters */
  if (code >= KEY_Q && code <= KEY_P)
    use_shift ^= caps_lock;
  if (code >= KEY_A && code <= KEY_L)
    use_shift ^= caps_lock;
  if (code >= KEY_Z && code <= KEY_M)
    use_shift ^= caps_lock;

  if (use_shift)
    c = scancode_to_ascii_shift[code];
  else
    c = scancode_to_ascii[code];

  /* Per-keystroke logging is debug-only: at pr_info level every press AND
   * release printed a line from IRQ context, flooding the serial log and
   * serialising all CPUs on the printk path while typing. */
  if (c != 0) {
    pr_debug("Keyboard: Char='%c' (val=%d) -> PID %d\n", c, value,
             keyboard_focus_pid);
  }

  /* Send IPC message if we have a focus PID */
  if (keyboard_focus_pid > 0) {
    struct ipc_message msg;
    memset(&msg, 0, sizeof(msg));
    msg.from = 0; /* Kernel/Driver */
    msg.type = IPC_TYPE_INPUT;
    msg.data1 = ((uint64_t)code << 16) | (uint8_t)c;
    msg.data2 = (uint64_t)value; /* 0=release, 1=press, 2=repeat */

    /* UTF-8 Handling */
    if (c != 0) {
      msg.payload[0] = c;
      msg.payload[1] = '\0';
    }

    /* Apply Layout Overrides */
    if (value != 0 && current_layout) {
      for (int i = 0; i < 16 && current_layout->utf8_overrides[i].utf8 != NULL;
           i++) {
        if (current_layout->utf8_overrides[i].code == code &&
            current_layout->utf8_overrides[i].shifted == shift_pressed) {
          strlcpy(msg.payload, current_layout->utf8_overrides[i].utf8,
                  sizeof(msg.payload));
          break;
        }
      }
    }

    kernel_ipc_send(keyboard_focus_pid, &msg);
  }
}

/*
 * Poll keyboard for new input
 */
void keyboard_poll(void) {
  struct virtio_input_event event;

  while (virtio_input_poll(&event)) {
    if (event.type == EV_KEY) {
      keyboard_process_key(event.code, event.value);
    }
  }
}

/*
 * Check if keyboard has buffered input
 */
/*
 * Read one character from keyboard buffer (non-blocking) - DEPRECATED
 * Standard input should now be handled via IPC messages.
 */
int keyboard_read_char_nonblock(void) { return -1; }

/*
 * Read one character from keyboard buffer (blocking) - DEPRECATED
 */
char keyboard_read_char(void) { return 0; }

/*
 * Read a line of input (blocking, with echo)
 */
int keyboard_read_line(char *buf, int max_len) {
  int i = 0;

  while (i < max_len - 1) {
    char c = keyboard_read_char();

    if (c == '\n' || c == '\r') {
      buf[i] = '\0';
      return i;
    }

    if (c == '\b' || c == 127) {
      if (i > 0) {
        i--;
        /* Echo backspace */
        printk("\b \b");
      }
      continue;
    }

    if (c >= 32 && c < 127) {
      buf[i++] = c;
      /* Echo character */
      printk("%c", c);
    }
  }

  buf[i] = '\0';
  return i;
}
