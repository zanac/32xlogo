/*
 * shot.c - minimal headless libretro frontend used to test the ROM.
 *
 * Runs a core (e.g. PicoDrive) for N frames with no input and writes a PPM
 * screenshot every K frames. No audio, no window.
 *
 * Build (needs libretro.h, e.g. from the PicoDrive tree):
 *   cc -O1 -o shot tools/headless/shot.c \
 *      -I<picodrive>/platform/libretro/libretro-common/include -ldl
 * Run:
 *   ./shot picodrive_libretro.so build/logo.32x outdir 300 10
 *   -> outdir/f00000.ppm, f00010.ppm, ...
 */
#include <stdio.h>
#include <stdlib.h>
#include <stdbool.h>
#include <dlfcn.h>
#include "libretro.h"

static unsigned pixel_format = RETRO_PIXEL_FORMAT_0RGB1555;
static const char *outdir;
static int frame, every;

static bool environment(unsigned cmd, void *data)
{
    switch (cmd) {
    case RETRO_ENVIRONMENT_SET_PIXEL_FORMAT:
        pixel_format = *(const unsigned *)data;
        return true;
    case RETRO_ENVIRONMENT_GET_SYSTEM_DIRECTORY:
    case RETRO_ENVIRONMENT_GET_SAVE_DIRECTORY:
        *(const char **)data = "/tmp";
        return true;
    default:
        return false;
    }
}

static void video(const void *data, unsigned w, unsigned h, size_t pitch)
{
    char name[1024];
    FILE *f;
    unsigned x, y;

    if (!data || frame % every)
        return;
    snprintf(name, sizeof(name), "%s/f%05d.ppm", outdir, frame);
    if (!(f = fopen(name, "wb")))
        return;
    fprintf(f, "P6 %u %u 255\n", w, h);
    for (y = 0; y < h; y++)
        for (x = 0; x < w; x++) {
            const char *line = (const char *)data + y * pitch;
            unsigned char rgb[3];
            if (pixel_format == RETRO_PIXEL_FORMAT_XRGB8888) {
                unsigned p = ((const unsigned *)line)[x];
                rgb[0] = p >> 16; rgb[1] = p >> 8; rgb[2] = p;
            } else {                                    /* RGB565 */
                unsigned p = ((const unsigned short *)line)[x];
                rgb[0] = ((p >> 11) & 31) << 3;
                rgb[1] = ((p >> 5) & 63) << 2;
                rgb[2] = (p & 31) << 3;
            }
            fwrite(rgb, 1, 3, f);
        }
    fclose(f);
}

static void audio(int16_t l, int16_t r) { (void)l; (void)r; }
static size_t audio_batch(const int16_t *d, size_t n) { (void)d; return n; }
static void input_poll(void) { }
static int16_t input_state(unsigned p, unsigned d, unsigned i, unsigned id)
{ (void)p; (void)d; (void)i; (void)id; return 0; }

#define LOAD(sym) typeof(&sym) p_##sym = dlsym(core, #sym)

int main(int argc, char **argv)
{
    void *core;
    int frames;

    if (argc != 6) {
        fprintf(stderr, "usage: %s core.so rom outdir frames every\n", argv[0]);
        return 1;
    }
    if (!(core = dlopen(argv[1], RTLD_NOW))) {
        fprintf(stderr, "%s\n", dlerror());
        return 1;
    }
    outdir = argv[3];
    frames = atoi(argv[4]);
    every  = atoi(argv[5]) > 0 ? atoi(argv[5]) : 1;

    LOAD(retro_set_environment); LOAD(retro_set_video_refresh);
    LOAD(retro_set_audio_sample); LOAD(retro_set_audio_sample_batch);
    LOAD(retro_set_input_poll); LOAD(retro_set_input_state);
    LOAD(retro_init); LOAD(retro_load_game); LOAD(retro_run);

    p_retro_set_environment(environment);
    p_retro_init();
    p_retro_set_video_refresh(video);
    p_retro_set_audio_sample(audio);
    p_retro_set_audio_sample_batch(audio_batch);
    p_retro_set_input_poll(input_poll);
    p_retro_set_input_state(input_state);

    struct retro_game_info game = { argv[2], NULL, 0, NULL };
    if (!p_retro_load_game(&game)) {
        fprintf(stderr, "cannot load %s\n", argv[2]);
        return 1;
    }
    for (frame = 0; frame < frames; frame++)
        p_retro_run();
    return 0;
}
