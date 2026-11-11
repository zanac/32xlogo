| ============================================================================
|  crt0.s - 68000 side: cartridge header, 32X boot header and start-up code
| ============================================================================
|
|  Cartridge layout (offsets in the ROM file; the 68000 sees them at +$880000
|  once the 32X adapter is enabled):
|
|    $000  68000 vector table (used only before the 32X takes over)
|    $100  Mega Drive header (system string, names, checksum, ...)
|    $200  jump table: the 32X vector ROM sends exception n to $880200+(n-1)*6
|    $3C0  32X header: where the SH-2 program is and where it starts
|    $3F0  standard 32X boot/security code (ICD_MARS, assets/icd_mars.bin)
|    $800  our code: _start (the boot code jumps here with the 32X up)
|
|  Syntax: GNU as, MIT/Motorola mix with "%" register prefix.
| ============================================================================

        .equ    ROM_BASE,   0x880000        | cart ROM as seen with the 32X on
        .equ    SDRAM,      0x06000000      | SH-2 SDRAM (cached)

        .section .header, "ax"

| ---------------------------------------------------------------------------
| $000: vector table. Reset jumps straight to the 32X boot code at $3F0.
| ---------------------------------------------------------------------------
        .long   0x00000000                  | 0  initial SSP (boot code sets it)
        .long   0x000003F0                  | 1  initial PC  -> ICD_MARS
        .rept   22
        .long   exc_halt                    | 2-23  bus/address/illegal/...
        .endr
        .rept   8
        .long   int_ignore                  | 24-31 spurious + IRQ level 1-7
        .endr
        .rept   16
        .long   int_ignore                  | 32-47 TRAP #0-15
        .endr
        .rept   16
        .long   exc_halt                    | 48-63 reserved
        .endr

| ---------------------------------------------------------------------------
| $100: Mega Drive header. The checksum word is patched by tools/mkrom.py.
| ---------------------------------------------------------------------------
        .org    0x100
        .ascii  "SEGA 32X        "                                  | system (required)
        .ascii  "(C)ZANAC2026.NOV"                                  | copyright
        .ascii  "LOGO 32X                                        "  | JP name
        .ascii  "LOGO 32X                                        "  | int. name
        .ascii  "GM 00000000-00"                                    | serial
        .word   0x0000                                              | checksum
        .ascii  "J               "                                  | I/O
        .long   0x00000000, 0x0003FFFF                              | ROM range
        .long   0x00FF0000, 0x00FFFFFF                              | RAM range
        .ascii  "            "                                      | no SRAM
        .ascii  "            "                                      | modem
        .ascii  "                                        "          | notes
        .ascii  "JUE             "                                  | regions

| ---------------------------------------------------------------------------
| $200: 32X jump table. Entry n-1 is used for exception vector n (n = 1..47).
| We run with interrupts masked, so nearly everything is a "should not
| happen" halt; entry 0 (hot start = reset button) restarts the program.
| ---------------------------------------------------------------------------
        .org    0x200
        jmp     (_start + 0).l              | 1  hot start
        .rept   22
        jmp     (exc_halt).l                | 2-23
        .endr
        .rept   24
        jmp     (int_ignore).l              | 24-47 (IRQs and traps)
        .endr

| ---------------------------------------------------------------------------
| $3C0: 32X header, read by the SH-2 boot ROMs.
| They copy <size> bytes from ROM offset <src> to SDRAM+<dst>, set VBR and
| jump to the entry points. The SH-2 binary (src/sh2) is linked at $06000000
| with a fixed layout: vectors at +$000, master entry +$200, slave +$220.
| ---------------------------------------------------------------------------
        .org    0x3C0
        .ascii  "MARS CHECK MODE "          | module name
        .long   0x00000000                  | version
        .long   sh2_image - ROM_BASE        | source offset in ROM
        .long   0x00000000                  | destination offset in SDRAM
        .long   sh2_image_end - sh2_image   | size
        .long   SDRAM + 0x200               | master SH-2 entry
        .long   SDRAM + 0x220               | slave SH-2 entry
        .long   SDRAM                       | master VBR
        .long   SDRAM                       | slave VBR

| ---------------------------------------------------------------------------
| $3F0: standard 32X boot code. It enables the adapter, unlocks TMSS,
| waits for both SH-2s ("M_OK"/"S_OK") and finally jumps to $800 (= $880800)
| with the carry flag set if something failed.
| ---------------------------------------------------------------------------
        .org    0x3F0
        .incbin "icd_mars.bin"

| ---------------------------------------------------------------------------
| $800: our entry point.
| ---------------------------------------------------------------------------
        .org    0x800
        .globl  _start
_start:
        move.w  #0x2700, %sr                | supervisor, all IRQs masked
        lea     0x01000000, %sp             | stack at top of work RAM

        | copy .data from ROM to RAM
        lea     _data_load, %a0
        lea     _data_start, %a1
        move.l  #_data_end, %d0
        sub.l   %a1, %d0
        lsr.l   #1, %d0
        bra.s   2f
1:      move.w  (%a0)+, (%a1)+
2:      dbra    %d0, 1b

        | clear .bss
        lea     _bss_start, %a1
        move.l  #_bss_end, %d0
        sub.l   %a1, %d0
        lsr.l   #1, %d0
        moveq   #0, %d1
        bra.s   4f
3:      move.w  %d1, (%a1)+
4:      dbra    %d0, 3b

        jsr     main
        jmp     _start                      | main never returns

| ---------------------------------------------------------------------------
| Exception handlers
| ---------------------------------------------------------------------------
exc_halt:
        move.w  #0x2700, %sr
        bra.s   exc_halt                    | crash: stop here (debugger-friendly)

int_ignore:
        rte

| ---------------------------------------------------------------------------
| The SH-2 program, built separately (src/sh2) and embedded as raw binary.
| ---------------------------------------------------------------------------
        .section .rodata.sh2, "a"
        .balign 4
sh2_image:
        .incbin "sh2.bin"
        .balign 4
sh2_image_end:
