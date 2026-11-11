#!/bin/sh
# ============================================================================
#  build_sh_toolchain.sh - build an sh-elf (SH-2, big endian) C cross compiler
#
#  Distributions ship SH-4 Linux compilers that cannot emit SH-2 code (-m2),
#  so the SH-2 side needs its own small gcc. This builds binutils + a C-only
#  gcc + libgcc, nothing else (no newlib: the demo is freestanding).
#
#  Usage:  tools/build_sh_toolchain.sh [PREFIX] [JOBS]
#          PREFIX defaults to $HOME/sh-elf, JOBS to the number of CPUs.
#  Then:   export PATH=PREFIX/bin:$PATH
#
#  Needs: a host C/C++ compiler, make, wget or curl, and the GMP/MPFR/MPC
#  development packages (Debian/Ubuntu: libgmp-dev libmpfr-dev libmpc-dev).
#  Tip: on Debian/Ubuntu "apt install binutils-sh-elf" provides the binutils
#  part; set SKIP_BINUTILS=1 to reuse it.
# ============================================================================
set -e

PREFIX=${1:-$HOME/sh-elf}
JOBS=${2:-$(getconf _NPROCESSORS_ONLN 2>/dev/null || echo 2)}
BINUTILS=binutils-2.42
GCC=gcc-13.3.0
MIRROR=https://ftp.gnu.org/gnu
WORK=${WORK:-$(pwd)/sh-toolchain-build}

fetch() {
    [ -f "$2" ] && return 0
    if command -v wget >/dev/null; then wget -O "$2" "$1"; else curl -L -o "$2" "$1"; fi
}

mkdir -p "$WORK" "$PREFIX"
cd "$WORK"
export PATH="$PREFIX/bin:$PATH"

if [ -z "$SKIP_BINUTILS" ]; then
    fetch "$MIRROR/binutils/$BINUTILS.tar.xz" "$BINUTILS.tar.xz"
    [ -d "$BINUTILS" ] || tar xf "$BINUTILS.tar.xz"
    mkdir -p build-binutils && cd build-binutils
    "../$BINUTILS/configure" --target=sh-elf --prefix="$PREFIX" \
        --disable-nls --disable-werror
    make -j"$JOBS" && make install
    cd ..
else
    # reuse the system sh-elf binutils
    mkdir -p "$PREFIX/bin"
    for t in as ld ar ranlib nm objcopy objdump strip; do
        ln -sf "$(command -v sh-elf-$t)" "$PREFIX/bin/sh-elf-$t"
    done
fi

fetch "$MIRROR/gcc/$GCC/$GCC.tar.xz" "$GCC.tar.xz"
[ -d "$GCC" ] || tar xf "$GCC.tar.xz"
mkdir -p build-gcc && cd build-gcc
"../$GCC/configure" --target=sh-elf --prefix="$PREFIX" \
    --with-cpu=m2 --with-endian=big --disable-multilib \
    --enable-languages=c --without-headers --with-newlib \
    --disable-nls --disable-shared --disable-threads --disable-libssp \
    --disable-libquadmath --disable-libgomp --disable-libatomic
make -j"$JOBS" all-gcc all-target-libgcc
make install-gcc install-target-libgcc

echo "done: export PATH=$PREFIX/bin:\$PATH"
