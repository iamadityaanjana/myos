[BITS 32]
global isr_panic

isr_panic:
    cli
.hang:
    hlt
    jmp .hang
