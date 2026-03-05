[BITS 32]

extern isr_handler
extern irq_handler

global isr0
global isr1
global isr2
global isr3
global isr4
global isr5
global isr6
global isr7
global isr8
global isr9
global isr10
global isr11
global isr12
global isr13
global isr14
global isr15
global isr16
global isr17
global isr18
global isr19
global isr20
global isr21
global isr22
global isr23
global isr24
global isr25
global isr26
global isr27
global isr28
global isr29
global isr30
global isr31

global irq0
global irq1
global irq2
global irq3
global irq4
global irq5
global irq6
global irq7
global irq8
global irq9
global irq10
global irq11
global irq12
global irq13
global irq14
global irq15

%macro ISR_NOERR 2
%1:
    push dword 0
    push dword %2
    jmp isr_common_stub
%endmacro

%macro ISR_ERR 2
%1:
    push dword %2
    jmp isr_common_stub
%endmacro

%macro IRQ_STUB 2
%1:
    push dword 0
    push dword %2
    jmp irq_common_stub
%endmacro

ISR_NOERR isr0, 0
ISR_NOERR isr1, 1
ISR_NOERR isr2, 2
ISR_NOERR isr3, 3
ISR_NOERR isr4, 4
ISR_NOERR isr5, 5
ISR_NOERR isr6, 6
ISR_NOERR isr7, 7
ISR_ERR   isr8, 8
ISR_NOERR isr9, 9
ISR_ERR   isr10, 10
ISR_ERR   isr11, 11
ISR_ERR   isr12, 12
ISR_ERR   isr13, 13
ISR_ERR   isr14, 14
ISR_NOERR isr15, 15
ISR_NOERR isr16, 16
ISR_ERR   isr17, 17
ISR_NOERR isr18, 18
ISR_NOERR isr19, 19
ISR_NOERR isr20, 20
ISR_NOERR isr21, 21
ISR_NOERR isr22, 22
ISR_NOERR isr23, 23
ISR_NOERR isr24, 24
ISR_NOERR isr25, 25
ISR_NOERR isr26, 26
ISR_NOERR isr27, 27
ISR_NOERR isr28, 28
ISR_NOERR isr29, 29
ISR_ERR   isr30, 30
ISR_NOERR isr31, 31

IRQ_STUB irq0, 32
IRQ_STUB irq1, 33
IRQ_STUB irq2, 34
IRQ_STUB irq3, 35
IRQ_STUB irq4, 36
IRQ_STUB irq5, 37
IRQ_STUB irq6, 38
IRQ_STUB irq7, 39
IRQ_STUB irq8, 40
IRQ_STUB irq9, 41
IRQ_STUB irq10, 42
IRQ_STUB irq11, 43
IRQ_STUB irq12, 44
IRQ_STUB irq13, 45
IRQ_STUB irq14, 46
IRQ_STUB irq15, 47

isr_common_stub:
    pusha
    cld
    push esp
    call isr_handler
    add esp, 4
    popa
    add esp, 8
    iretd

irq_common_stub:
    pusha
    cld
    push esp
    call irq_handler
    add esp, 4

    test eax, eax
    jz .use_current_irq_frame
    mov esp, eax

.use_current_irq_frame:

    popa
    add esp, 8
    iretd
