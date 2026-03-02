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
KERNEL_A := kernel/kernel_entry.asm

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

# ── Kernel C object ───────────────────────────────────────────
$(BUILD)/kernel.o: $(KERNEL_C) | $(BUILD)
	$(CC) $(CFLAGS) -c $< -o $@

# ── Link kernel into flat binary ──────────────────────────────
$(BUILD)/kernel.bin: $(BUILD)/kernel_entry.o $(BUILD)/kernel.o
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
