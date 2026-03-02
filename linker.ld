ENTRY(_start)

SECTIONS
{
  /*
   * Kernel is loaded at physical address 0x1000 by the bootloader.
   * The linker (not NASM ORG) sets this base address.
   */
  . = 0x1000;

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
}
