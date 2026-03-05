#include "screen.h"
#include "gdt.h"
#include "idt.h"
#include "printk.h"
#include "keyboard.h"
#include "shell.h"
#include "memory.h"
#include "paging.h"
#include "scheduler.h"
#include "timer.h"

void kernel_main() {
    clear_screen();

    set_color(0x0A);
    print("=================================\n");
    print("        MyOS v0.1\n");
    print("=================================\n\n");

    set_color(0x07);
    gdt_init();
    memory_init();
    paging_init();
    scheduler_init();
    keyboard_init();

    // print("Screen driver initialized.\n");
    // print("Setting up IDT...\n");
    idt_init();

    // print("Remapping PIC...\n");
    pic_remap();
    timer_init(100);

    print("Enabling interrupts...\n");
    __asm__ volatile("sti");
    print("Interrupts enabled. Keyboard ready.\n");
    shell_init();

    // print("Keyboard IRQ active (full keymap enabled).\n");
    // print("Try letters, digits, symbols, shift, caps, backspace.\n");
    // print("print_int tests: ");
    // print_int(0);
    // print(" ");
    // print_int(12345);
    // print(" ");
    // print_int(-42);
    // put_char('\n');

    while (1) {
        __asm__ volatile("hlt");
        keyboard_process_pending();
    }
}
