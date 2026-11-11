#!/usr/bin/env python3
"""
mkrom.py - turn the linked 68000 binary into a finished 32X ROM.

Usage:
    python3 tools/mkrom.py <in.bin> <out.32x>

  * pads the image to a multiple of 128 KB (with $FF, like an erased EPROM)
  * writes the ROM end address in the header ($1A4)
  * computes the Mega Drive checksum (sum of all words from $200) into $18E.
    The 32X boot code compares it with the value the SH-2 boot ROM computes,
    so it must be right.
"""
import struct
import sys

PAD = 128 * 1024


def main():
    if len(sys.argv) != 3:
        sys.exit(__doc__)
    rom = bytearray(open(sys.argv[1], "rb").read())
    if rom[0x100:0x108] != b"SEGA 32X" or rom[0x3C0:0x3D0] != b"MARS CHECK MODE ":
        sys.exit("error: headers are not where they should be")

    size = (len(rom) + PAD - 1) // PAD * PAD
    rom += b"\xff" * (size - len(rom))

    struct.pack_into(">I", rom, 0x1A4, size - 1)
    checksum = sum(struct.unpack(">%dH" % ((size - 0x200) // 2), rom[0x200:])) & 0xFFFF
    struct.pack_into(">H", rom, 0x18E, checksum)

    open(sys.argv[2], "wb").write(rom)
    print("ok: %s, %d KB, checksum %04X" % (sys.argv[2], size // 1024, checksum))


if __name__ == "__main__":
    main()
