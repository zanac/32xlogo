/*
 * hw.h - Mega Drive VDP and 32X adapter registers, as seen by the 68000.
 */
#ifndef HW_H
#define HW_H

typedef unsigned char  u8;
typedef unsigned short u16;
typedef unsigned int   u32;
typedef signed short   s16;

#define REG16(a) (*(volatile u16 *)(a))
#define REG32(a) (*(volatile u32 *)(a))

/* ---- Mega Drive VDP ------------------------------------------------------ */
#define VDP_DATA       REG16(0xC00000)
#define VDP_CTRL       REG16(0xC00004)
#define VDP_CTRL32     REG32(0xC00004)
#define PSG_PORT       (*(volatile u8 *)0xC00011)

#define VDP_STATUS_VBLANK  0x0008

/* write a VDP register */
#define VDP_SET_REG(r, v)  (VDP_CTRL = (u16)(0x8000 | ((r) << 8) | ((v) & 0xFF)))

/* "command" long words that select the address of the next data port access */
#define VDP_CMD(code, addr) \
    ((((u32)(code) & 3) << 30) | (((u32)(addr) & 0x3FFF) << 16) | \
     (((u32)(code) & 0x3C) << 2) | (((u32)(addr) >> 14) & 3))

#define VDP_VRAM_WRITE(addr)   VDP_CMD(0x01, addr)
#define VDP_CRAM_WRITE(addr)   VDP_CMD(0x03, addr)
#define VDP_VSRAM_WRITE(addr)  VDP_CMD(0x05, addr)

/* ---- 32X adapter (68000 side, $A15100) ----------------------------------- */
#define MARS_ADAPTER_CTRL  REG16(0xA15100)   /* bit 15: FM = SH-2 owns 32X VDP */
#define MARS_COMM0_32      REG32(0xA15120)   /* COMM0+COMM2 as one long */
#define MARS_COMM4_32      REG32(0xA15124)   /* COMM4+COMM6 as one long */

#define MARS_FM            0x8000

/* written by the SH-2 boot ROMs once the program has been loaded */
#define MARS_M_OK          0x4D5F4F4B        /* "M_OK" */
#define MARS_S_OK          0x535F4F4B        /* "S_OK" */

#endif
