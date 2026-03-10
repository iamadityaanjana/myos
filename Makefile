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
GDT_C    := kernel/gdt.c
IDT_C    := kernel/idt.c
ISR_C    := kernel/isr.c
KBD_C    := kernel/keyboard.c
PRINTK_C := kernel/printk.c
SHELL_C  := kernel/shell.c
MEM_C    := kernel/memory.c
PAGING_C := kernel/paging.c
RTC_C    := kernel/rtc.c
TIMER_C  := kernel/timer.c
SCHED_C  := kernel/scheduler.c
RAMFS_C  := kernel/ramfs.c
DISK_C   := kernel/disk.c
PFS_C    := kernel/pfs.c
MOUSE_C  := kernel/mouse.c
FB_C     := kernel/framebuffer.c
COMP_C   := kernel/compositor.c
KERNEL_A := kernel/kernel_entry.asm
GDT_A    := kernel/gdt_flush.asm
IDT_A    := kernel/idt_load.asm
INT_A    := kernel/interrupt_stubs.asm

.PHONY: all clean run run-fullscreen

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

# ── GDT flush object ─────────────────────────────────────────
$(BUILD)/gdt_flush.o: $(GDT_A) | $(BUILD)
	$(AS) -f elf32 $< -o $@

# ── Interrupt stubs object ───────────────────────────────────
$(BUILD)/interrupt_stubs.o: $(INT_A) | $(BUILD)
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

# ── GDT object ───────────────────────────────────────────────
$(BUILD)/gdt.o: $(GDT_C) | $(BUILD)
	$(CC) $(CFLAGS) -c $< -o $@

# ── IDT object ───────────────────────────────────────────────
$(BUILD)/idt.o: $(IDT_C) | $(BUILD)
	$(CC) $(CFLAGS) -c $< -o $@

# ── ISR dispatcher object ────────────────────────────────────
$(BUILD)/isr.o: $(ISR_C) | $(BUILD)
	$(CC) $(CFLAGS) -c $< -o $@

# ── Keyboard object ──────────────────────────────────────────
$(BUILD)/keyboard.o: $(KBD_C) | $(BUILD)
	$(CC) $(CFLAGS) -c $< -o $@

# ── printk object ────────────────────────────────────────────
$(BUILD)/printk.o: $(PRINTK_C) | $(BUILD)
	$(CC) $(CFLAGS) -c $< -o $@

# ── shell object ─────────────────────────────────────────────
$(BUILD)/shell.o: $(SHELL_C) | $(BUILD)
	$(CC) $(CFLAGS) -c $< -o $@

# ── memory object ────────────────────────────────────────────
$(BUILD)/memory.o: $(MEM_C) | $(BUILD)
	$(CC) $(CFLAGS) -c $< -o $@

# ── paging object ────────────────────────────────────────────
$(BUILD)/paging.o: $(PAGING_C) | $(BUILD)
	$(CC) $(CFLAGS) -c $< -o $@

# ── rtc object ───────────────────────────────────────────────
$(BUILD)/rtc.o: $(RTC_C) | $(BUILD)
	$(CC) $(CFLAGS) -c $< -o $@

# ── timer object ─────────────────────────────────────────────
$(BUILD)/timer.o: $(TIMER_C) | $(BUILD)
	$(CC) $(CFLAGS) -c $< -o $@

# ── scheduler object ─────────────────────────────────────────
$(BUILD)/scheduler.o: $(SCHED_C) | $(BUILD)
	$(CC) $(CFLAGS) -c $< -o $@

# ── ramfs object ─────────────────────────────────────────────
$(BUILD)/ramfs.o: $(RAMFS_C) | $(BUILD)
	$(CC) $(CFLAGS) -c $< -o $@

# ── disk object ──────────────────────────────────────────────
$(BUILD)/disk.o: $(DISK_C) | $(BUILD)
	$(CC) $(CFLAGS) -c $< -o $@

# ── persistent fs object ─────────────────────────────────────
$(BUILD)/pfs.o: $(PFS_C) | $(BUILD)
	$(CC) $(CFLAGS) -c $< -o $@

# ── mouse object ─────────────────────────────────────────────
$(BUILD)/mouse.o: $(MOUSE_C) | $(BUILD)
	$(CC) $(CFLAGS) -c $< -o $@

# ── framebuffer object ───────────────────────────────────────
$(BUILD)/framebuffer.o: $(FB_C) | $(BUILD)
	$(CC) $(CFLAGS) -c $< -o $@

# ── compositor object ────────────────────────────────────────
$(BUILD)/compositor.o: $(COMP_C) | $(BUILD)
	$(CC) $(CFLAGS) -c $< -o $@

# ── Link kernel into flat binary ──────────────────────────────
$(BUILD)/kernel.bin: $(BUILD)/kernel_entry.o $(BUILD)/gdt_flush.o $(BUILD)/idt_load.o $(BUILD)/interrupt_stubs.o $(BUILD)/kernel.o $(BUILD)/screen.o $(BUILD)/io.o $(BUILD)/gdt.o $(BUILD)/idt.o $(BUILD)/isr.o $(BUILD)/keyboard.o $(BUILD)/printk.o $(BUILD)/shell.o $(BUILD)/memory.o $(BUILD)/paging.o $(BUILD)/rtc.o $(BUILD)/timer.o $(BUILD)/scheduler.o $(BUILD)/ramfs.o $(BUILD)/disk.o $(BUILD)/pfs.o $(BUILD)/mouse.o $(BUILD)/framebuffer.o $(BUILD)/compositor.o
	$(LD) $(LDFLAGS) -o $@ $^ --oformat binary

# ── Persistent data disk image (16 MiB) ─────────────────────
$(BUILD)/data.img: | $(BUILD)
	@if [ ! -f $@ ]; then \
		dd if=/dev/zero of=$@ bs=1M count=16 status=none; \
		echo "Created persistent data disk -> $@"; \
	fi

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
run: $(BUILD)/os.img $(BUILD)/data.img
	qemu-system-i386 -fda $(BUILD)/os.img -drive file=$(BUILD)/data.img,format=raw,if=ide

# ── Launch in fullscreen (display-scaled) ─────────────────────
run-fullscreen: $(BUILD)/os.img $(BUILD)/data.img
	qemu-system-i386 -fda $(BUILD)/os.img -drive file=$(BUILD)/data.img,format=raw,if=ide -full-screen

# ── Clean ─────────────────────────────────────────────────────
clean:
	rm -rf $(BUILD)
