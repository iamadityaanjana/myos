#ifndef ISR_H
#define ISR_H

#include "types.h"

typedef struct registers {
    uint32_t edi;
    uint32_t esi;
    uint32_t ebp;
    uint32_t esp;
    uint32_t ebx;
    uint32_t edx;
    uint32_t ecx;
    uint32_t eax;
    uint32_t int_no;
    uint32_t err_code;
    uint32_t eip;
    uint32_t cs;
    uint32_t eflags;
} registers_t;

void isr_handler(registers_t* regs);
uint32_t irq_handler(registers_t* regs);

#endif
