# Logo — 32X

A tiny 32X ROM, written in C, that boots on the 32X and shows only an
animated **logo** (fade in, blue "shine", fade out), without sound.

All assets are in `assets/`, so the repository builds out of the box:

```
make
# -> build/logo.32x  (128 KB)
```

---

## How the logo is drawn

The logo is not on the 32X frame buffer. It is drawn by the **Mega Drive VDP**
using 7 sprites, with the animation done by rewriting CRAM. The 32X layer is
switched off.

Tiles: 98 tiles (3136 bytes), uploaded to VRAM `$9000` (tile `$480`).

Sprites (screen coordinates, H40 320×224):

| # | Position | Size | Tiles | Palette | Part |
|---|---|---|---|---|---|
| 0–2 | (112, 93), (144, 93), (176, 93) | 32×32 each | `$4B2`, `$4C2`, `$4D2` | 0 | blue letters (front) |
| 3–5 | (111, 92), (143, 92), (175, 92) | 32×32 each | `$480`, `$490`, `$4A0` | 1 | metallic outline (behind, 1 px up-left) |
| 6 | (207, 116) | 16×8 | `$4B0` | 1 | "TM" |

Animation:

| Phase | Behaviour |
|---|---|
| Fade in | 16 steps, 1 per frame: channel = max(0, target − (step+1)), step 15 → 0 |
| Shine | every 3 frames colours 1–15 of palette 0 rotate by one position |
| Fade out | every channel moves 1 step toward black per frame, 16 frames |

## How the demo works

```
reset ─► $3F0 32X boot code: adapter on, TMSS, SH-2s start
          │                         │
          │                SH-2 boot ROMs copy build/sh2.bin to SDRAM $06000000,
          │                write "M_OK"/"S_OK", jump to $06000200 / $06000220
          ▼
       $800 _start (src/md/crt0.s) → main() (src/md/main.c)
          • waits for M_OK + S_OK, sets FM=1 (32X VDP to the SH-2s), clears COMM0
          • VDP setup (H40), uploads tiles to $9000, writes the 7 sprites
          • fade in → shine (forever, or the full sequence)
                                    │
          master SH-2 (src/sh2/main.c): waits COMM0 == 0,
          sets the 32X VDP to blank mode, then sleeps. Slave: sleeps.
```

Everything runs with interrupts masked. Frame timing comes from polling the VDP
V-blank status bit.

By default the logo fades in and the shine keeps running forever. With
`make FULL_SEQUENCE=1` the demo plays the complete intro (fade in, 135 frames
of shine, fade out), then pauses for 60 black frames and loops.

### Cartridge layout

| Offset | Content | Source |
|---|---|---|
| `$000` | 68000 vectors (PC = `$3F0`) | `src/md/crt0.s` |
| `$100` | Mega Drive header (system string required by the console); checksum and ROM end are patched by `tools/mkrom.py` | `src/md/crt0.s` |
| `$200` | 32X jump table: the 32X vector ROM sends exception *n* to `$880200 + (n−1)·6` | `src/md/crt0.s` |
| `$3C0` | `MARS CHECK MODE` header: SH-2 image offset/size, entries `$06000200`/`$06000220`, VBR `$06000000` | `src/md/crt0.s` |
| `$3F0` | standard 32X boot code | `assets/icd_mars.bin` |
| `$800` | 68000 program, logo data, embedded SH-2 image | `src/md/*.c`, `assets/logo_data.c`, `build/sh2.bin` |

## Repository

```
Makefile                      build (all paths/tools can be set on the command line)
assets/logo_data.c            logo tiles + palette
assets/icd_mars.bin           standard 32X boot code, $3F0-$7FF
src/md/crt0.s                 68000: vectors, headers, jump table, start-up
src/md/main.c                 68000: VDP setup, sprites, fade/shine animation
src/md/hw.h                   VDP and 32X register definitions
src/md/logo_data.h            interface of assets/logo_data.c
src/md/md.ld                  68000 linker script (ROM at $880000, RAM at $FF0000)
src/sh2/crt0.s                SH-2: vector table, master/slave entry points
src/sh2/main.c                SH-2: hand-shake, 32X layer off, idle
src/sh2/sh2.ld                SH-2 linker script (SDRAM $06000000)
tools/mkrom.py                pads to 128 KB, writes ROM end and checksum
tools/build_sh_toolchain.sh   builds the sh-elf (SH-2) gcc
tools/headless/shot.c         headless libretro frontend for screenshots
```

## Requirements

- **68000:** `m68k-elf-gcc`, or a distribution's `m68k-linux-gnu-gcc`
  (`apt install gcc-m68k-linux-gnu`). The code does not need libgcc, so
  `-m68000` with the Linux cross compiler works. Override with
  `M68K_PREFIX=...`.
- **SH-2:** `sh-elf-gcc` that supports `-m2 -mb`. Distribution SH-4 compilers
  do not, so build one with `tools/build_sh_toolchain.sh` (on Debian/Ubuntu,
  `apt install binutils-sh-elf libgmp-dev libmpfr-dev libmpc-dev`, then
  `SKIP_BINUTILS=1 tools/build_sh_toolchain.sh`). Override with
  `SH_PREFIX=...`.
- **Python 3** (for `tools/mkrom.py`).

Example with explicit paths:

```
make M68K_PREFIX=/opt/m68k/bin/m68k-elf- SH_PREFIX=$HOME/sh-elf/bin/sh-elf- \
     PYTHON=/usr/bin/python3
```

## Testing

Tested in PicoDrive (libretro core, also the revision shipped with
Batocera 41) with `tools/headless/shot.c`: 60 Hz with region on Auto, palette
rotation exactly every 3 frames, no missed frames.

```
cc -O1 -o shot tools/headless/shot.c -I<picodrive>/platform/libretro/libretro-common/include -ldl
./shot picodrive_libretro.so build/logo.32x /tmp/shots 300 10
```

Not yet tested on real hardware.
