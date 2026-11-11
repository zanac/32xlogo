/* sh2.ld - SH-2 program, copied by the boot ROM to SDRAM $06000000 */
OUTPUT_FORMAT("elf32-sh")
OUTPUT_ARCH(sh)
ENTRY(_vectors)

MEMORY
{
    sdram (rwx) : ORIGIN = 0x06000000, LENGTH = 0x3E000   /* stacks above */
}

SECTIONS
{
    .text : {
        KEEP(*(.vectors))          /* must be first: VBR table + entries */
        *(.text .text.*)
        *(.rodata .rodata.*)
        *(.data .data.*)           /* the whole image is copied to SDRAM, */
        . = ALIGN(4);              /* so .data needs no extra copy        */
    } > sdram

    .bss (NOLOAD) : {
        _bss_start = .;
        *(.bss .bss.* COMMON)
        . = ALIGN(4);
        _bss_end = .;
    } > sdram

    /DISCARD/ : { *(.comment) *(.note*) }
}
