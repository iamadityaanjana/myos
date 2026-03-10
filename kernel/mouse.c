#include "mouse.h"

#include "io.h"
#include "shell.h"

#define MOUSE_QUEUE_SIZE 128

static volatile unsigned char queue[MOUSE_QUEUE_SIZE];
static volatile unsigned char queue_head = 0;
static volatile unsigned char queue_tail = 0;

static int mouse_x = 40;
static int mouse_y = 12;
static int packet_index = 0;
static unsigned char packet[3];
static int prev_left = 0;
static int prev_right = 0;

static int mouse_wait_read() {
    for (int i = 0; i < 100000; i++) {
        if (inb(0x64) & 1) {
            return 1;
        }
    }
    return 0;
}

static int mouse_wait_write() {
    for (int i = 0; i < 100000; i++) {
        if ((inb(0x64) & 2) == 0) {
            return 1;
        }
    }
    return 0;
}

static void mouse_write(unsigned char value) {
    if (!mouse_wait_write()) {
        return;
    }
    outb(0x64, 0xD4);
    if (!mouse_wait_write()) {
        return;
    }
    outb(0x60, value);
}

static unsigned char mouse_read() {
    if (!mouse_wait_read()) {
        return 0;
    }
    return inb(0x60);
}

void mouse_init() {
    if (!mouse_wait_write()) {
        return;
    }
    outb(0x64, 0xA8);

    if (!mouse_wait_write()) {
        return;
    }
    outb(0x64, 0x20);
    unsigned char status = mouse_read();
    status |= 0x02;

    if (!mouse_wait_write()) {
        return;
    }
    outb(0x64, 0x60);
    if (!mouse_wait_write()) {
        return;
    }
    outb(0x60, status);

    mouse_write(0xF6);
    (void)mouse_read();

    mouse_write(0xF4);
    (void)mouse_read();
}

void mouse_irq_handler() {
    unsigned char data = inb(0x60);
    unsigned char next = (unsigned char)((queue_head + 1) % MOUSE_QUEUE_SIZE);
    if (next != queue_tail) {
        queue[queue_head] = data;
        queue_head = next;
    }
}

void mouse_process_pending() {
    while (queue_tail != queue_head) {
        unsigned char b = queue[queue_tail];
        queue_tail = (unsigned char)((queue_tail + 1) % MOUSE_QUEUE_SIZE);

        if (packet_index == 0 && ((b & 0x08) == 0)) {
            continue;
        }

        packet[packet_index++] = b;
        if (packet_index < 3) {
            continue;
        }
        packet_index = 0;

        int dx = (int)((signed char)packet[1]);
        int dy = (int)((signed char)packet[2]);

        mouse_x += dx / 2;
        mouse_y -= dy / 4;

        if (mouse_x < 0) mouse_x = 0;
        if (mouse_x > 79) mouse_x = 79;
        if (mouse_y < 0) mouse_y = 0;
        if (mouse_y > 24) mouse_y = 24;

        int left = (packet[0] & 0x01) ? 1 : 0;
        int right = (packet[0] & 0x02) ? 1 : 0;

        int left_pressed = left && !prev_left;
        int left_released = !left && prev_left;
        int right_pressed = right && !prev_right;
        int right_released = !right && prev_right;

        shell_mouse_event(mouse_x, mouse_y,
                          left, right,
                          left_pressed, left_released,
                          right_pressed, right_released);

        prev_left = left;
        prev_right = right;
    }
}
