#include "io.h"
#include "screen.h"

#define KBD_QUEUE_SIZE 64

static int shift_pressed = 0;
static int caps_lock = 0;
static int extended_prefix = 0;
static volatile uint8_t scancode_queue[KBD_QUEUE_SIZE];
static volatile uint8_t queue_head = 0;
static volatile uint8_t queue_tail = 0;

static const char keymap[128] = {
    0,
    27,
    '1', '2', '3', '4', '5', '6', '7', '8', '9', '0', '-', '=',
    '\b',
    '\t',
    'q', 'w', 'e', 'r', 't', 'y', 'u', 'i', 'o', 'p', '[', ']',
    '\n',
    0,
    'a', 's', 'd', 'f', 'g', 'h', 'j', 'k', 'l', ';', '\'', '`',
    0,
    '\\',
    'z', 'x', 'c', 'v', 'b', 'n', 'm', ',', '.', '/',
    0,
    '*',
    0,
    ' ',
};

static const char keymap_shift[128] = {
    0,
    27,
    '!', '@', '#', '$', '%', '^', '&', '*', '(', ')', '_', '+',
    '\b',
    '\t',
    'Q', 'W', 'E', 'R', 'T', 'Y', 'U', 'I', 'O', 'P', '{', '}',
    '\n',
    0,
    'A', 'S', 'D', 'F', 'G', 'H', 'J', 'K', 'L', ':', '"', '~',
    0,
    '|',
    'Z', 'X', 'C', 'V', 'B', 'N', 'M', '<', '>', '?',
    0,
    '*',
    0,
    ' ',
};

static int is_alpha(char c) {
    return (c >= 'a' && c <= 'z') || (c >= 'A' && c <= 'Z');
}

static void put_tab() {
    for (int i = 0; i < 4; i++) {
        put_char(' ');
    }
}

static void process_scancode(uint8_t scancode) {
    if (scancode == 0xE0) {
        extended_prefix = 1;
        return;
    }

    if (extended_prefix) {
        extended_prefix = 0;

        if (scancode == 0x48) {
            scroll_view_up(1);
        } else if (scancode == 0x50) {
            scroll_view_down(1);
        }
        return;
    }

    if (scancode == 0x2A || scancode == 0x36) {
        shift_pressed = 1;
        return;
    }

    if (scancode == 0xAA || scancode == 0xB6) {
        shift_pressed = 0;
        return;
    }

    if (scancode == 0x3A) {
        caps_lock = !caps_lock;
        return;
    }

    if (scancode & 0x80) {
        return;
    }

    char c = 0;

    if (scancode < 128) {
        char normal = keymap[scancode];
        char shifted = keymap_shift[scancode];

        if (is_alpha(normal)) {
            if (shift_pressed ^ caps_lock) {
                c = shifted;
            } else {
                c = normal;
            }
        } else {
            c = shift_pressed ? shifted : normal;
        }
    }

    if (c == '\t') {
        put_tab();
    } else if (c != 0) {
        put_char(c);
    }
}

void keyboard_irq_handler() {
    uint8_t scancode = inb(0x60);
    uint8_t next_head = (uint8_t)((queue_head + 1) % KBD_QUEUE_SIZE);

    if (next_head != queue_tail) {
        scancode_queue[queue_head] = scancode;
        queue_head = next_head;
    }
}

void keyboard_process_pending() {
    while (queue_tail != queue_head) {
        uint8_t scancode = scancode_queue[queue_tail];
        queue_tail = (uint8_t)((queue_tail + 1) % KBD_QUEUE_SIZE);
        process_scancode(scancode);
    }
}
