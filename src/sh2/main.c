/*
 * main.c - SH-2 programs (master and slave).
 *
 * The logo is drawn by the Mega Drive VDP (see src/md/main.c). The 32X picture is composited on top of it, so the only job of the
 * SH-2 side is to make sure the 32X layer is switched off (bitmap mode 0 =
 * blank) and then stay idle.
 */

typedef unsigned short u16;
typedef unsigned int   u32;

/* 32X system registers, cache-through area */
#define MARS_REG16(off)    (*(volatile u16 *)(0x20004000 + (off)))
#define MARS_COMM(n)       MARS_REG16(0x20 + 2 * (n))
#define MARS_VDP_MODE      MARS_REG16(0x100)   /* bit 0-1: 0 = blank */
#define MARS_PALETTE       ((volatile u16 *)0x20004200)

void master_main(void)
{
    int i;

    /* the boot ROM left "M_OK" in COMM0; the 68000 clears it once it has
     * seen both "M_OK" and "S_OK" and has handed the 32X VDP to us (FM = 1) */
    while (MARS_COMM(0) != 0)
        ;

    MARS_VDP_MODE = 0;                  /* 32X layer off */
    for (i = 0; i < 256; i++)           /* and a black 32X palette anyway */
        MARS_PALETTE[i] = 0;

    for (;;)
        __asm__ volatile ("sleep");
}

void slave_main(void)
{
    for (;;)
        __asm__ volatile ("sleep");
}
