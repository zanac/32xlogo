/*
 * logo_data.h - logo assets (data in assets/logo_data.c).
 */
#ifndef LOGO_DATA_H
#define LOGO_DATA_H

/* 98 tiles (4bpp, 8x8, 32 bytes each) as big-endian VRAM words.
 * They are uploaded at VRAM $9000, i.e. starting at tile $480. */
extern const unsigned short logo_tile_words;
extern const unsigned short logo_tiles[];

/* 32 colours in Mega Drive format (----BBB-GGG-RRR-; some values carry an
 * extra low bit per channel, which the VDP ignores):
 *   [ 0..15] palette 0: blue gradient, rotated every 3 frames ("shine")
 *   [16..31] palette 1: grey/white metallic outline and the "TM" sign   */
extern const unsigned short logo_palette[32];

#endif
