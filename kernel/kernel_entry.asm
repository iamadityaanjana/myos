[BITS 32]

global _start
extern kernel_main

_start:
    mov al, 'S'
    out 0xE9, al

    cld

    ; Set up stack at 0x70000
    mov esp, 0x70000

    mov al, 'T'
    out 0xE9, al

    ; Call the C kernel
    mov al, 'C'
    out 0xE9, al

    call kernel_main

    mov al, 'R'
    out 0xE9, al

    ; Hang forever if kernel returns
    cli
.hang:
    hlt
    jmp .hang
