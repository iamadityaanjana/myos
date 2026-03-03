[BITS 32]

global _start
extern kernel_main

_start:
    cld

    ; Set up stack at 0x90000
    mov esp, 0x90000

    ; Call the C kernel
    call kernel_main

    ; Hang forever if kernel returns
    cli
.hang:
    hlt
    jmp .hang
