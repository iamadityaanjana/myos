#include "idt.h"
#include "io.h"
#include "types.h"

typedef struct {
    uint16_t offset_low;
    uint16_t selector;
    uint8_t zero;
    uint8_t type_attr;
    uint16_t offset_high;
} __attribute__((packed)) IDTEntry;

typedef struct {
    uint16_t limit;
    uint32_t base;
} __attribute__((packed)) IDTPointer;

static IDTEntry idt[256];
static IDTPointer idt_ptr;

extern void idt_load(uint32_t ptr);
extern void keyboard_isr();
extern void irq_default_isr();
extern void isr_panic();

static void idt_set_gate(int n, uint32_t handler) {
    idt[n].offset_low = (uint16_t)(handler & 0xFFFF);
    idt[n].selector = 0x08;
    idt[n].zero = 0;
    idt[n].type_attr = 0x8E;
    idt[n].offset_high = (uint16_t)((handler >> 16) & 0xFFFF);
}

void pic_remap() {
    outb(0x20, 0x11);
    io_wait();
    outb(0xA0, 0x11);
    io_wait();

    outb(0x21, 0x20);
    io_wait();
    outb(0xA1, 0x28);
    io_wait();

    outb(0x21, 0x04);
    io_wait();
    outb(0xA1, 0x02);
    io_wait();

    outb(0x21, 0x01);
    io_wait();
    outb(0xA1, 0x01);
    io_wait();

    outb(0x21, 0xFD);
    io_wait();
    outb(0xA1, 0xFF);
    io_wait();
}

void idt_init() {
    for (int i = 0; i < 256; i++) {
        idt[i].offset_low = 0;
        idt[i].selector = 0;
        idt[i].zero = 0;
        idt[i].type_attr = 0;
        idt[i].offset_high = 0;
    }

    for (int i = 0; i < 32; i++) {
        idt_set_gate(i, (uint32_t)isr_panic);
    }

    for (int i = 32; i < 48; i++) {
        idt_set_gate(i, (uint32_t)irq_default_isr);
    }

    idt_set_gate(33, (uint32_t)keyboard_isr);

    idt_ptr.limit = (uint16_t)(sizeof(idt) - 1);
    idt_ptr.base = (uint32_t)&idt;

    idt_load((uint32_t)&idt_ptr);
}
