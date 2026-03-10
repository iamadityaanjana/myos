# MyOS

A hobby 32-bit x86 operating system project that boots from a floppy image, runs in QEMU, and currently includes:

- Bootloader (real mode -> protected mode)
- Basic kernel subsystems (GDT, IDT, ISRs, IRQ handling)
- Keyboard and mouse input
- Timer and simple scheduler primitives
- Memory + paging setup
- Persistent file system support (`pfs`)
- UI desktop mode with shell/folder windows in VGA graphics mode

This README is written for someone cloning the repo and running it for the first time.

## Requirements

You need:

- `nasm`
- `qemu-system-i386`
- `make`
- A 32-bit cross compiler toolchain:
  - `i686-elf-gcc`
  - `i686-elf-ld`

The Makefile uses these exact tool names:

- `AS := nasm`
- `CC := i686-elf-gcc`
- `LD := i686-elf-ld`

If they are not in your `PATH`, build will fail.

## macOS Setup

### 1. Install base tools

```bash
brew install nasm qemu make
```

### 2. Install/build i686-elf cross toolchain

If you already have `i686-elf-gcc` and `i686-elf-ld`, skip this.

Common approach is to build binutils+gcc cross toolchain manually.

At minimum, verify:

```bash
which i686-elf-gcc
which i686-elf-ld
```

Both must resolve to real executables.

## Linux Setup (Ubuntu/Debian example)

```bash
sudo apt update
sudo apt install -y nasm qemu-system-x86 make build-essential
```

Then install or build `i686-elf-gcc` + `i686-elf-ld` (cross toolchain) similarly.

## Quick Start

From project root:

```bash
make clean && make
dd if=/dev/zero of=build/data.img bs=1M count=16 status=none
make run
```

What happens:

- `make` builds `build/os.img` (boot floppy image)
- `data.img` is used as persistent disk backing for file data
- `make run` launches QEMU with both images attached

## Build Targets

- `make` or `make all`
  - Builds `build/os.img`
- `make run`
  - Runs QEMU with `build/os.img` + `build/data.img`
- `make clean`
  - Removes `build/`

Note: `make clean` removes `build/data.img` too, so recreate it before `make run`.

## Current Boot/Runtime Flow

1. `boot/bootloader.asm` loads kernel sectors from floppy
2. Switches to protected mode
3. Jumps to kernel entry
4. Kernel initializes low-level subsystems
5. Starts UI desktop mode (graphics)

## Desktop UI (Current)

On boot, desktop shows two icons:

- `Shell`
- `Folders`

### Shell window

- Click Shell icon to open
- Shell window is fullscreen inside UI canvas
- Type commands and press Enter
- Use Up/Down arrows to scroll shell output

Supported UI shell commands currently include:

- `help`
- `clear`
- `ls`
- `cat FILE`
- `hexdump FILE`
- `touch FILE`
- `write FILE TEXT`
- `rm FILE`

### Folders window

- Click Folders icon to open
- Shows current file list from persistent FS (`pfs`)
- Updates after shell file operations

### Mouse behavior

- Click inside QEMU window to capture mouse
- On macOS, QEMU title bar usually shows release shortcut

## Resolution and Fullscreen Notes

Current graphics path is VGA mode 13h (`320x200`).

Important:

- Fullscreen QEMU window does not mean native HD rendering
- It scales 320x200 to your monitor
- Native 1920x1080 requires VBE/LFB implementation (not yet complete)

## Project Structure

- `boot/`
  - `bootloader.asm` : stage-1 loader and mode transition
- `kernel/`
  - core kernel, drivers, FS, shell, UI/compositor modules
- `linker.ld`
  - kernel link layout
- `Makefile`
  - build and run orchestration
- `presentation/`
  - project presentation materials
- `build/`
  - generated artifacts (`os.img`, objects, etc.)

## Troubleshooting

### 1) `i686-elf-gcc: command not found`

You are missing cross compiler in PATH.
Install/build toolchain and export PATH.

### 2) QEMU opens but black/blank screen

Checklist:

- Rebuild clean:
  ```bash
  make clean && make
  ```
- Recreate persistent disk:
  ```bash
  dd if=/dev/zero of=build/data.img bs=1M count=16 status=none
  ```
- Relaunch:
  ```bash
  make run
  ```

Also close old QEMU instances first.

### 3) No typing visible in shell window

- Open the Shell icon first
- Click inside QEMU so keyboard is captured
- Ensure shell window is focused/visible

### 4) `build/data.img` missing

Create it manually:

```bash
dd if=/dev/zero of=build/data.img bs=1M count=16 status=none
```

### 5) Fullscreen still looks small

Expected with current VGA mode 13h backend.
Needs future VBE/LFB work for true higher resolution.

## Development Notes

- Kernel is freestanding (`-ffreestanding`, `-nostdlib`, `-nostdinc`)
- Keep changes simple and test with `make clean && make`
- For debugging boot flow, `0xE9` debug console breadcrumbs are present in current code

## Suggested Workflow

```bash
# 1) edit code
# 2) rebuild
make clean && make

# 3) ensure data disk exists
[ -f build/data.img ] || dd if=/dev/zero of=build/data.img bs=1M count=16 status=none

# 4) run
make run
```

## License

No explicit license file is currently present in this repository.
Add one if you plan to share/distribute publicly.
