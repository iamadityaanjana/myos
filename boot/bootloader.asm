[BITS 16]
[ORG 0x7C00]

start:
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

    ; Load kernel: 16 sectors starting at sector 2 → address 0x1000
    ; ES:BX = 0x0000:0x1000
    xor ax, ax
    mov es, ax           ; ensure ES=0 for the destination address
    mov bx, 0x1000       ; physical address = ES*16 + BX = 0x1000
    mov ah, 0x02         ; BIOS read
    mov al, 16           ; sectors to read
    mov ch, 0            ; cylinder 0
    mov cl, 2            ; start at sector 2
    mov dh, 0            ; head 0
    mov dl, [boot_drive] ; drive number saved from BIOS
    int 0x13
    jc disk_error

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
    mov ax, 0x10            ; data segment selector
    mov ds, ax
    mov es, ax
    mov fs, ax
    mov gs, ax
    mov ss, ax
    mov esp, 0x90000        ; stack below 640 KB

    jmp 0x1000              ; jump to loaded kernel binary

msg_boot     db "Booting MyOS...", 13, 10, 0
msg_disk_err db "Disk read error!", 13, 10, 0
boot_drive   db 0

times 510 - ($ - $$) db 0
dw 0xAA55
