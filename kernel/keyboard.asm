[BITS 32]
global keyboard_isr
extern keyboard_handler

keyboard_isr:
    pusha
    cld
    call keyboard_handler
    popa
    iretd
