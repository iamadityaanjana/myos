[BITS 16]
[ORG 0x7C00]

KERNEL_MAX_SECTORS equ 256
KERNEL_LOAD_SEGMENT equ 0x1000
KERNEL_LOAD_OFFSET  equ 0x0000
KERNEL_LOAD_ADDR    equ 0x10000

start:
    mov al, 'A'
    out 0xE9, al

    ; BIOS puts boot drive number in DL — save it before we touch anything
    mov [boot_drive], dl

    cli
    xor ax, ax
    mov ds, ax
    mov es, ax
    mov ss, ax
    mov sp, 0x7C00
    sti                  ; re-enable interrupts — BIOS int 0x13 needs them

    ; Print boot message (16-bit BIOS teletype)
    mov si, msg_boot
    call print16

    ; Reset disk controller first (AH=0)
    xor ah, ah
    mov dl, [boot_drive]
    int 0x13
    jc disk_error

    ; Enable A20 line via fast A20 port
    call enable_a20

    ; Load kernel from floppy using CHS progression.
    ; We intentionally read a fixed upper bound from sector 1 onward
    ; so kernel growth does not break boot as quickly.
    mov ax, KERNEL_LOAD_SEGMENT
    mov es, ax
    mov bx, KERNEL_LOAD_OFFSET

    mov word [sectors_remaining], KERNEL_MAX_SECTORS
    mov byte [chs_cylinder], 0
    mov byte [chs_head], 0
    mov byte [chs_sector], 2      ; first kernel sector is LBA 1 => CHS 0/0/2

.read_kernel_loop:
    mov al, 'R'
    out 0xE9, al

    cmp word [sectors_remaining], 0
    je .kernel_loaded

    mov ch, [chs_cylinder]
    mov dh, [chs_head]
    mov cl, [chs_sector]
    mov ah, 0x02
    mov al, 1
    mov dl, [boot_drive]
    int 0x13
    jc disk_error

    add bx, 512
    jnc .no_segment_bump
    mov ax, es
    add ax, 0x1000
    mov es, ax

.no_segment_bump:
    dec word [sectors_remaining]

    inc byte [chs_sector]
    cmp byte [chs_sector], 19
    jb .read_kernel_loop

    mov byte [chs_sector], 1
    xor byte [chs_head], 1
    cmp byte [chs_head], 0
    jne .read_kernel_loop

.next_cylinder:
    inc byte [chs_cylinder]
    cmp byte [chs_cylinder], 80
    jb .read_kernel_loop
    jmp disk_error

.kernel_loaded:
    mov al, 'L'
    out 0xE9, al

    mov si, msg_loaded
    call print16

    ; Enter graphics mode 13h (320x200x256 linear framebuffer at 0xA0000).
    mov ax, 0x0013
    int 0x10

    ; Disable interrupts before touching GDT/CR0 — no IDT exists in PM yet
    cli

    ; Load GDT and enter Protected Mode
    lgdt [gdt_descriptor]
    mov eax, cr0
    or  eax, 0x1
    mov cr0, eax

    ; Far jump flushes prefetch queue, fully enters 32-bit code segment
    jmp 0x08:pm_entry

disk_error:
    mov si, msg_disk_err
    call print16
    hlt

print16:
    mov ah, 0x0E
.loop:
    lodsb
    test al, al
    jz .done
    int 0x10
    jmp .loop
.done:
    ret

enable_a20:
    in  al, 0x92
    or  al, 0x02
    out 0x92, al
    ret

; ── Global Descriptor Table ───────────────────────────────────
gdt_start:
    dq 0                    ; null descriptor

    ; Code segment  base=0  limit=4 GB  32-bit  ring 0
    dw 0xFFFF               ; limit[15:0]
    dw 0x0000               ; base[15:0]
    db 0x00                 ; base[23:16]
    db 10011010b            ; access: P=1 DPL=0 S=1 E=1 DC=0 RW=1 A=0
    db 11001111b            ; flags: G=1 DB=1 L=0 + limit[19:16]=0xF
    db 0x00                 ; base[31:24]

    ; Data segment  base=0  limit=4 GB  32-bit  ring 0
    dw 0xFFFF
    dw 0x0000
    db 0x00
    db 10010010b            ; access: P=1 DPL=0 S=1 E=0 DC=0 RW=1 A=0
    db 11001111b
    db 0x00
gdt_end:

gdt_descriptor:
    dw gdt_end - gdt_start - 1
    dd gdt_start

; ── 32-bit Protected Mode entry ───────────────────────────────
[BITS 32]
pm_entry:
    mov al, 'P'
    out 0xE9, al

    mov ax, 0x10            ; data segment selector
    mov ds, ax
    mov es, ax
    mov fs, ax
    mov gs, ax
    mov ss, ax
    mov esp, 0x70000        ; stack in safe conventional RAM region

    mov al, 'J'
    out 0xE9, al

    mov eax, KERNEL_LOAD_ADDR
    jmp eax                 ; absolute jump to loaded kernel binary

msg_boot     db "Booting MyOS...", 13, 10, 0
msg_loaded   db "Kernel loaded", 13, 10, 0
msg_disk_err db "Disk read error!", 13, 10, 0
boot_drive   db 0
sectors_remaining dw 0
chs_cylinder db 0
chs_head db 0
chs_sector db 0

times 510 - ($ - $$) db 0
dw 0xAA55
