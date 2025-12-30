#!/bin/bash

set -e

function build() {
  pushd 7zip/$2
  nproc=$(nproc)
  rm -rf "_o"

  case "$ARCH" in
    x86_64)
      make CC=$CC CXX=$CXX -j$((nproc-1)) -f makefile.gcc IS_X64=1 USE_ASM=1 MY_ASM=uasm || exit 1
      ;;
    i386)
      make CC=$CC CXX=$CXX -j$((nproc-1)) -f makefile.gcc IS_X86=1 USE_ASM=0
      ;;
    aarch64)
      make CC=$CC CXX=$CXX -j$((nproc-1)) -f makefile.gcc IS_ARM64=1 USE_ASM=0
      ;;
    *)
      echo "Unknown arch: $ARCH"
      exit 1
      ;;
  esac

  # strip down for releases
  strip _o/$1
  cp _o/$1 "$OUTDIR"
  popd
}

export OUTDIR="$PWD/build"
export ARCH=$(arch)
export CC="gcc"
export CXX="g++"

export CC="clang"
export CXX="clang++"
mkdir -p "$OUTDIR"

# standalone, minimalistic (flzma2, zstd)
build 7zr   Bundles/Alone7z

# standalone, small (flzma2, zstd, lz4, hashes)
build 7za   Bundles/Alone

# standalone, full featured
build 7zz   Bundles/Alone2

# full featured via plugin loading (7z.so)
build 7z    UI/Console
build 7z.so Bundles/Format7zF
