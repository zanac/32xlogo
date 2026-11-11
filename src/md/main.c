/*
 * main.c - 68000 program: shows the animated logo, without sound.
 *
 *   - The logo is NOT on the 32X frame buffer: it is 7 Mega Drive VDP sprites
 *     using tiles $480-$4E1 (VRAM $9000). Two 96x32 layers made of three
 *     32x32 sprites each, plus the 16x8 "TM":
 *        layer 1 (palette 1) = grey outline,        top-left  at (111, 92)
 *        layer 0 (palette 0) = blue letters, 1px lower-right at (112, 93)
 *     Sprites 0-2 (blue) have priority over 3-5 (outline).
 *   - Fade in: 16 steps, one per frame. At step s (15..0) every 4-bit
 *     channel of the target colour is reduced by s+1 (clamped at 0).
 *   - Shine: every 3 frames colours 1..15 of palette 0 rotate by one
 *     position (colour 1 goes to 15).
 *   - Fade out: every frame each channel moves 1 step toward black,
 *     16 frames.
 *
 * By default the demo fades in and then keeps the shine running forever.
 * Build with -DFULL_SEQUENCE (make FULL_SEQUENCE=1) to play the complete
 * intro instead (fade in, 135 frames of shine, fade out) in a loop.
 */
#include "hw.h"
#include "logo_data.h"

/* ---- screen / VRAM layout ------------------------------------------------- */
#define PLANE_A_ADDR   0xC000
#define WINDOW_ADDR    0xB000
#define PLANE_B_ADDR   0xE000
#define SPRITES_ADDR   0xF000
#define HSCROLL_ADDR   0xFC00
#define LOGO_VRAM_ADDR 0x9000        /* tiles start at ... */
#define LOGO_TILE      (LOGO_VRAM_ADDR / 32)   /* ... = tile $480 */

/* ---- timing (frames) --------------------------------------------------------- */
#define FADE_STEPS        16
#define FADE_IN_FRAMES     1         /* frames per fade-in step */
#define SHINE_PERIOD       3         /* frames per palette rotation */
#define SHINE_FRAMES     135         /* length of the shine phase */
#define PAUSE_FRAMES      60         /* black pause before looping (ours) */

/* ---- sprites ---------------------------------------------------------------- */
struct sprite {
    s16 x, y;            /* screen position (top-left) */
    u8  w, h;            /* size in tiles (1..4) */
    u16 tile;            /* first tile, relative to LOGO_TILE */
    u8  pal;             /* palette line */
};

static const struct sprite logo_sprites[] = {
    /* palette 0: blue letters (in front) */
    { 112, 93, 4, 4, 0x32, 0 },      /* left third */
    { 144, 93, 4, 4, 0x42, 0 },      /* middle third */
    { 176, 93, 4, 4, 0x52, 0 },      /* right third */
    /* palette 1: metallic outline (behind, 1 pixel up-left) */
    { 111, 92, 4, 4, 0x00, 1 },
    { 143, 92, 4, 4, 0x10, 1 },
    { 175, 92, 4, 4, 0x20, 1 },
    /* palette 1: "TM" */
    { 207, 116, 2, 1, 0x30, 1 },
};
#define NUM_SPRITES ((u16)(sizeof(logo_sprites) / sizeof(logo_sprites[0])))

/* working copy of the 2 palettes that is uploaded to CRAM every frame */
static u16 palette[32];

/* -------------------------------------------------------------------------- */

static void wait_vblank(void)
{
    while (VDP_CTRL & VDP_STATUS_VBLANK)    /* leave the current vblank... */
        ;
    while (!(VDP_CTRL & VDP_STATUS_VBLANK)) /* ...and wait for the next one */
        ;
}

static void wait_frames(int n)
{
    while (n-- > 0)
        wait_vblank();
}

static void upload_palette(void)
{
    int i;
    VDP_CTRL32 = VDP_CRAM_WRITE(0);
    for (i = 0; i < 32; i++)
        VDP_DATA = palette[i];
}

static void vdp_init(void)
{
    u16 i;

    (void)VDP_CTRL;                 /* reset the command latch */

    VDP_SET_REG(0x00, 0x04);        /* no H-int */
    VDP_SET_REG(0x01, 0x04);        /* mode 5, display off while we load */
    VDP_SET_REG(0x02, PLANE_A_ADDR >> 10);
    VDP_SET_REG(0x03, WINDOW_ADDR >> 10);
    VDP_SET_REG(0x04, PLANE_B_ADDR >> 13);
    VDP_SET_REG(0x05, SPRITES_ADDR >> 9);
    VDP_SET_REG(0x07, 0x00);        /* backdrop = palette 0, colour 0 (black) */
    VDP_SET_REG(0x0A, 0xFF);
    VDP_SET_REG(0x0B, 0x00);        /* full-screen scrolling */
    VDP_SET_REG(0x0C, 0x81);        /* H40: 320x224 */
    VDP_SET_REG(0x0D, HSCROLL_ADDR >> 10);
    VDP_SET_REG(0x0F, 0x02);        /* auto-increment 2 */
    VDP_SET_REG(0x10, 0x01);        /* 64x32 planes */
    VDP_SET_REG(0x11, 0x00);        /* no window */
    VDP_SET_REG(0x12, 0x00);

    /* clear all of VRAM, VSRAM and CRAM: planes then show transparent tile 0 */
    VDP_CTRL32 = VDP_VRAM_WRITE(0);
    for (i = 0; i < 0x8000; i++)
        VDP_DATA = 0;
    VDP_CTRL32 = VDP_VSRAM_WRITE(0);
    for (i = 0; i < 40; i++)
        VDP_DATA = 0;
    VDP_CTRL32 = VDP_CRAM_WRITE(0);
    for (i = 0; i < 64; i++)
        VDP_DATA = 0;
}

static void load_logo(void)
{
    u16 i;

    VDP_CTRL32 = VDP_VRAM_WRITE(LOGO_VRAM_ADDR);
    for (i = 0; i < logo_tile_words; i++)
        VDP_DATA = logo_tiles[i];

    /* sprite attribute table: 4 words per sprite, linked in list order */
    VDP_CTRL32 = VDP_VRAM_WRITE(SPRITES_ADDR);
    for (i = 0; i < NUM_SPRITES; i++) {
        const struct sprite *s = &logo_sprites[i];
        u16 link = (i + 1 < NUM_SPRITES) ? i + 1 : 0;

        VDP_DATA = s->y + 128;
        VDP_DATA = ((s->w - 1) << 10) | ((s->h - 1) << 8) | link;
        VDP_DATA = (s->pal << 13) | (LOGO_TILE + s->tile);
        VDP_DATA = s->x + 128;
    }
}

/* Fade in: channel = max(0, target - (step + 1)), step = 15..0,
 * then the exact palette. Channels are the 3 nibbles of the colour word. */
static void fade_in(void)
{
    int step, i, sh;

    for (step = FADE_STEPS - 1; step >= -1; step--) {
        wait_frames(FADE_IN_FRAMES);
        for (i = 0; i < 32; i++) {
            u16 target = logo_palette[i] & 0x0FFF, c = 0;
            for (sh = 0; sh <= 8; sh += 4) {
                int v = ((target >> sh) & 0xF) - (step + 1);
                if (v > 0)
                    c |= v << sh;
            }
            palette[i] = c;
        }
        upload_palette();
    }
}

/* Shine: rotate colours 1..15 of palette 0 by one position. */
static void shine_step(void)
{
    u16 first = palette[1];
    int i;

    for (i = 1; i < 15; i++)
        palette[i] = palette[i + 1];
    palette[15] = first;
}

/* One frame of the shine phase: rotate on the first frame and then every
 * SHINE_PERIOD frames. */
static void shine_frame(int *counter)
{
    wait_vblank();
    if (--*counter < 0) {
        *counter = SHINE_PERIOD - 1;
        shine_step();
    }
    upload_palette();
}

#ifdef FULL_SEQUENCE
/* Fade out: each channel moves one step toward black per frame. */
static void fade_out(void)
{
    int n, i, sh;

    for (n = 0; n < FADE_STEPS; n++) {
        wait_vblank();
        for (i = 0; i < 32; i++) {
            u16 c = palette[i] & 0x0FFF;
            for (sh = 0; sh <= 8; sh += 4)
                if ((c >> sh) & 0xF)
                    c -= 1 << sh;
            palette[i] = c;
        }
        upload_palette();
    }
}
#endif

/* Hand-shake with the SH-2s. Their boot ROMs write "M_OK"/"S_OK" into
 * COMM0/COMM4 before jumping to our SH-2 code; we wait for both, give the
 * 32X VDP to the SH-2 side (FM = 1) and clear COMM0: that is the "go" for
 * the master, which then switches the 32X layer off (blank mode) so only the
 * Mega Drive picture - our logo - is visible. */
static void start_sh2(void)
{
    while (MARS_COMM0_32 != MARS_M_OK)
        ;
    while (MARS_COMM4_32 != MARS_S_OK)
        ;
    MARS_ADAPTER_CTRL |= MARS_FM;
    MARS_COMM4_32 = 0;
    MARS_COMM0_32 = 0;
}

static void silence_psg(void)
{
    PSG_PORT = 0x9F;                /* channel 0..3 volume = off */
    PSG_PORT = 0xBF;
    PSG_PORT = 0xDF;
    PSG_PORT = 0xFF;
}

void main(void)
{
    int counter, n;

    silence_psg();
    start_sh2();
    vdp_init();
    load_logo();
    VDP_SET_REG(0x01, 0x44);        /* display on (V28, no IRQs, no DMA) */

    for (;;) {
        fade_in();
        counter = 0;
#ifdef FULL_SEQUENCE
        for (n = 0; n < SHINE_FRAMES; n++)
            shine_frame(&counter);
        fade_out();
        wait_frames(PAUSE_FRAMES);
#else
        (void)n;
        for (;;)
            shine_frame(&counter);
#endif
    }
}
