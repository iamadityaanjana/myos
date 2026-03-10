ENTRY(_start)

SECTIONS
{
  /*
   * Kernel is loaded at physical address 0x10000 by the bootloader.
   * The linker (not NASM ORG) sets this base address.
   */
  . = 0x10000;
  _kernel_start = .;

  .text : {
    *(.text*)
  }

  .rodata : {
    *(.rodata*)
  }

  .data : {
    *(.data*)
  }

  .bss : {
    *(.bss*)
    *(COMMON)
  }

  _kernel_end = .;
}
