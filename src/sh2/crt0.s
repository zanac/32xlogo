! ============================================================================
!  crt0.s - SH-2 side start-up (master and slave)
! ============================================================================
!
!  The SH-2 boot ROMs copy this whole binary to SDRAM $06000000 (see the 32X
!  header in src/md/crt0.s) and jump to:
!     master: $06000200      slave: $06000220
!  Both CPUs use VBR = $06000000, i.e. the vector table below.
!
!  Everything runs with interrupts masked (SR.I = 15): the logo needs no
!  SH-2 work besides turning the 32X layer off, which main.c does.
! ============================================================================

        .section .vectors, "ax"
        .global _vectors
_vectors:
        .long   master_entry            ! 0  power-on PC
        .long   0x06040000              ! 1  power-on SP
        .long   master_entry            ! 2  manual reset PC
        .long   0x06040000              ! 3  manual reset SP
        .rept   124
        .long   exc_halt                ! 4..127 errors, traps, IRQs
        .endr

! --- master: $06000200 ------------------------------------------------------
        .org    0x200
master_entry:
        mov.l   master_sp, r15
        mov.l   sr_mask, r0
        ldc     r0, sr
        mov.l   master_main, r0
        jsr     @r0
        nop
        bra     master_entry
        nop
        .align  2
master_sp:      .long   0x06040000      ! top of SDRAM
sr_mask:        .long   0x000000F0      ! I = 15
master_main:    .long   _master_main

! --- slave: $06000220 -------------------------------------------------------
        .org    0x220
slave_entry:
        mov.l   slave_sp, r15
        mov.l   sr_mask2, r0
        ldc     r0, sr
        mov.l   slave_main, r0
        jsr     @r0
        nop
        bra     slave_entry
        nop
        .align  2
slave_sp:       .long   0x0603F000      ! 4 KB below the master stack
sr_mask2:       .long   0x000000F0
slave_main:     .long   _slave_main

! --- unexpected exception: stay here --------------------------------------
exc_halt:
        bra     exc_halt
        nop
