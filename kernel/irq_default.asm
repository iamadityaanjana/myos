[BITS 32]
global irq_default_isr

irq_default_isr:
    push eax
    mov al, 0x20
    out 0xA0, al
    out 0x20, al
    pop eax
    iretd
