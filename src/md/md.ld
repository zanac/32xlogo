/* md.ld - 68000 program. The cartridge is seen at $880000 with the 32X on. */
OUTPUT_FORMAT("elf32-m68k")
OUTPUT_ARCH(m68k)
ENTRY(_start)

MEMORY
{
    rom (rx)  : ORIGIN = 0x880000, LENGTH = 0x80000     /* fixed 512 KB area */
    ram (rwx) : ORIGIN = 0xFF0000, LENGTH = 0xFF00      /* stack at the top  */
}

SECTIONS
{
    .text : {
        KEEP(*(.header))           /* vectors, headers, boot code, _start */
        *(.text .text.*)
        *(.rodata .rodata.*)
        . = ALIGN(4);
    } > rom

    .data : {
        _data_start = .;
        *(.data .data.*)
        . = ALIGN(4);
        _data_end = .;
    } > ram AT > rom
    _data_load = LOADADDR(.data);

    .bss (NOLOAD) : {
        _bss_start = .;
        *(.bss .bss.* COMMON)
        . = ALIGN(4);
        _bss_end = .;
    } > ram

    /DISCARD/ : { *(.comment) *(.note*) }
}
