#include "isr.h"
#include "io.h"
#include "screen.h"
#include "printk.h"
#include "keyboard.h"
#include "timer.h"

static const char* exception_messages[32] = {
    "Division By Zero",
    "Debug",
    "Non Maskable Interrupt",
    "Breakpoint",
    "Overflow",
    "Bound Range Exceeded",
    "Invalid Opcode",
    "Device Not Available",
    "Double Fault",
    "Coprocessor Segment Overrun",
    "Invalid TSS",
    "Segment Not Present",
    "Stack Segment Fault",
    "General Protection Fault",
    "Page Fault",
    "Reserved",
    "x87 Floating Point",
    "Alignment Check",
    "Machine Check",
    "SIMD Floating Point",
    "Virtualization",
    "Control Protection",
    "Reserved",
    "Reserved",
    "Reserved",
    "Reserved",
    "Reserved",
    "Reserved",
    "Hypervisor Injection",
    "VMM Communication",
    "Security",
    "Reserved"
};

void isr_handler(registers_t* regs) {
    volatile uint16_t* vga = (volatile uint16_t*)0xB8000;
    uint8_t color = 0x4F;
    uint32_t num = regs->int_no;
    char tens = (char)('0' + ((num / 10) % 10));
    char ones = (char)('0' + (num % 10));

    vga[0] = (uint16_t)((color << 8) | 'E');
    vga[1] = (uint16_t)((color << 8) | 'X');
    vga[2] = (uint16_t)((color << 8) | 'C');
    vga[3] = (uint16_t)((color << 8) | ' ');
    vga[4] = (uint16_t)((color << 8) | tens);
    vga[5] = (uint16_t)((color << 8) | ones);

    __asm__ volatile("cli");
    while (1) {
        __asm__ volatile("hlt");
    }
}

void irq_handler(registers_t* regs) {
    if (regs->int_no == 32) {
        timer_irq_handler();
    }

    if (regs->int_no == 33) {
        keyboard_irq_handler();
    }

    if (regs->int_no >= 40) {
        outb(0xA0, 0x20);
    }
    outb(0x20, 0x20);
}
