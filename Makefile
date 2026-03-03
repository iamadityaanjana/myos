# ─────────────────────────────────────────────────────────────
# MyOS Makefile  —  Minimal 32-bit x86 OS
# ─────────────────────────────────────────────────────────────

AS       := nasm
LD       := i686-elf-ld
CC       := i686-elf-gcc
CFLAGS   := -m32 -ffreestanding -fno-pie -fno-stack-protector \
            -nostdlib -nostdinc -O2 -Wall -Wextra
LDFLAGS  := -T linker.ld -nostdlib

BUILD    := build
BOOT_SRC := boot/bootloader.asm
KERNEL_C := kernel/kernel.c
SCREEN_C := kernel/screen.c
IO_C     := kernel/io.c
IDT_C    := kernel/idt.c
KBD_C    := kernel/keyboard.c
PRINTK_C := kernel/printk.c
KERNEL_A := kernel/kernel_entry.asm
IDT_A    := kernel/idt_load.asm
KBD_A    := kernel/keyboard.asm
IRQ_A    := kernel/irq_default.asm
PANIC_A  := kernel/isr_panic.asm

.PHONY: all clean run

all: $(BUILD)/os.img

# ── Create build directory ────────────────────────────────────
$(BUILD):
	mkdir -p $(BUILD)

# ── Bootloader (flat binary, 512 bytes) ───────────────────────
$(BUILD)/boot.bin: $(BOOT_SRC) | $(BUILD)
	$(AS) -f bin $< -o $@

# ── Kernel entry object (ELF32, no ORG — linker sets 0x1000) ──
$(BUILD)/kernel_entry.o: $(KERNEL_A) | $(BUILD)
	$(AS) -f elf32 $< -o $@

# ── IDT loader object ────────────────────────────────────────
$(BUILD)/idt_load.o: $(IDT_A) | $(BUILD)
	$(AS) -f elf32 $< -o $@

# ── Keyboard ISR stub object ─────────────────────────────────
$(BUILD)/keyboard_isr.o: $(KBD_A) | $(BUILD)
	$(AS) -f elf32 $< -o $@

# ── Default IRQ ISR object ───────────────────────────────────
$(BUILD)/irq_default.o: $(IRQ_A) | $(BUILD)
	$(AS) -f elf32 $< -o $@

# ── Panic ISR object (CPU exceptions) ────────────────────────
$(BUILD)/isr_panic.o: $(PANIC_A) | $(BUILD)
	$(AS) -f elf32 $< -o $@

# ── Kernel C object ───────────────────────────────────────────
$(BUILD)/kernel.o: $(KERNEL_C) | $(BUILD)
	$(CC) $(CFLAGS) -c $< -o $@

# ── Screen driver object ──────────────────────────────────────
$(BUILD)/screen.o: $(SCREEN_C) | $(BUILD)
	$(CC) $(CFLAGS) -c $< -o $@

# ── I/O object ───────────────────────────────────────────────
$(BUILD)/io.o: $(IO_C) | $(BUILD)
	$(CC) $(CFLAGS) -c $< -o $@

# ── IDT object ───────────────────────────────────────────────
$(BUILD)/idt.o: $(IDT_C) | $(BUILD)
	$(CC) $(CFLAGS) -c $< -o $@

# ── Keyboard object ──────────────────────────────────────────
$(BUILD)/keyboard.o: $(KBD_C) | $(BUILD)
	$(CC) $(CFLAGS) -c $< -o $@

# ── printk object ────────────────────────────────────────────
$(BUILD)/printk.o: $(PRINTK_C) | $(BUILD)
	$(CC) $(CFLAGS) -c $< -o $@

# ── Link kernel into flat binary ──────────────────────────────
$(BUILD)/kernel.bin: $(BUILD)/kernel_entry.o $(BUILD)/idt_load.o $(BUILD)/keyboard_isr.o $(BUILD)/irq_default.o $(BUILD)/isr_panic.o $(BUILD)/kernel.o $(BUILD)/screen.o $(BUILD)/io.o $(BUILD)/idt.o $(BUILD)/keyboard.o $(BUILD)/printk.o
	$(LD) $(LDFLAGS) -o $@ $^ --oformat binary

# ── Build 1.44 MB floppy image ──────────────────────────────
# Sector 0 = bootloader (512 bytes)
# Sector 1+ = kernel binary
# Rest      = zeros  (total 2880 sectors = 1.44 MB)
$(BUILD)/os.img: $(BUILD)/boot.bin $(BUILD)/kernel.bin
	# Create blank 1.44 MB image
	dd if=/dev/zero of=$@ bs=512 count=2880 status=none
	# Write bootloader at sector 0
	dd if=$(BUILD)/boot.bin of=$@ conv=notrunc bs=512 seek=0 status=none
	# Write kernel starting at sector 1
	dd if=$(BUILD)/kernel.bin of=$@ conv=notrunc bs=512 seek=1 status=none
	@echo ""
	@echo "Build complete → $(BUILD)/os.img"
	@echo "Run with:  make run"

# ── Launch in QEMU (floppy mode — no geometry issues) ─────────
run: $(BUILD)/os.img
	qemu-system-i386 -fda $(BUILD)/os.img

# ── Clean ─────────────────────────────────────────────────────
clean:
	rm -rf $(BUILD)
