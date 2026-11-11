# ============================================================================
#  Logo 32X demo - build
#
#    make
#
#  Options (all can be given on the command line):
#    FULL_SEQUENCE=1       play the complete intro (fade in, shine, fade out)
#                          in a loop instead of fade-in + endless shine
#    M68K_PREFIX=...       68000 cross tools  (default: m68k-elf- if found,
#                          otherwise m68k-linux-gnu-)
#    SH_PREFIX=...         SH-2 cross tools   (default: sh-elf-)
#    PYTHON=...            Python 3
#
#  Output: build/logo.32x
# ============================================================================

ASSETS      = assets
PYTHON      ?= python3
SH_PREFIX   ?= sh-elf-
M68K_PREFIX ?= $(if $(shell command -v m68k-elf-gcc 2>/dev/null),m68k-elf-,m68k-linux-gnu-)

M68K_CC      = $(M68K_PREFIX)gcc
M68K_LD      = $(M68K_PREFIX)ld
M68K_OBJCOPY = $(M68K_PREFIX)objcopy
SH_CC        = $(SH_PREFIX)gcc
SH_LD        = $(SH_PREFIX)ld
SH_OBJCOPY   = $(SH_PREFIX)objcopy

BUILD = build
OUT   = $(BUILD)/logo.32x

COMMON_CFLAGS = -O2 -Wall -Wextra -ffreestanding -fno-builtin -nostdlib \
                -fno-pic -fno-asynchronous-unwind-tables

M68K_CFLAGS = -m68000 $(COMMON_CFLAGS) -fomit-frame-pointer -Isrc/md
ifeq ($(FULL_SEQUENCE),1)
M68K_CFLAGS += -DFULL_SEQUENCE
endif
SH_CFLAGS   = -m2 -mb $(COMMON_CFLAGS)

.PHONY: all clean
all: $(OUT)

$(BUILD):
	mkdir -p $@

# ---- 1. SH-2 program -------------------------------------------------------
$(BUILD)/sh2_crt0.o: src/sh2/crt0.s | $(BUILD)
	$(SH_CC) $(SH_CFLAGS) -c $< -o $@

$(BUILD)/sh2_main.o: src/sh2/main.c | $(BUILD)
	$(SH_CC) $(SH_CFLAGS) -c $< -o $@

$(BUILD)/sh2.elf: $(BUILD)/sh2_crt0.o $(BUILD)/sh2_main.o src/sh2/sh2.ld
	$(SH_LD) -T src/sh2/sh2.ld -o $@ $(BUILD)/sh2_crt0.o $(BUILD)/sh2_main.o

$(BUILD)/sh2.bin: $(BUILD)/sh2.elf
	$(SH_OBJCOPY) -O binary $< $@

# ---- 2. 68000 program (embeds sh2.bin and icd_mars.bin) ------------------
$(BUILD)/md_crt0.o: src/md/crt0.s $(BUILD)/sh2.bin $(ASSETS)/icd_mars.bin
	$(M68K_CC) -m68000 -c -Wa,-I$(BUILD) -Wa,-I$(ASSETS) $< -o $@

$(BUILD)/md_main.o: src/md/main.c src/md/hw.h src/md/logo_data.h | $(BUILD)
	$(M68K_CC) $(M68K_CFLAGS) -c $< -o $@

$(BUILD)/md_logo_data.o: $(ASSETS)/logo_data.c src/md/logo_data.h | $(BUILD)
	$(M68K_CC) $(M68K_CFLAGS) -c $< -o $@

M68K_OBJS = $(BUILD)/md_crt0.o $(BUILD)/md_main.o $(BUILD)/md_logo_data.o

$(BUILD)/md.elf: $(M68K_OBJS) src/md/md.ld
	$(M68K_LD) -z noexecstack -T src/md/md.ld -o $@ $(M68K_OBJS)

$(BUILD)/md.bin: $(BUILD)/md.elf
	$(M68K_OBJCOPY) -O binary $< $@

# ---- 3. final ROM: padding, size, checksum ---------------------------------
$(OUT): $(BUILD)/md.bin tools/mkrom.py
	$(PYTHON) tools/mkrom.py $< $@

clean:
	rm -rf $(BUILD)
