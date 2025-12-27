#!/bin/bash

function build() {
  pushd 7zip/$2
  nproc=$(nproc)
  make CC=clang CXX=clang++ -j$((nproc-1)) -f makefile.gcc IS_X64=1 USE_ASM=1 MY_ASM=uasm
  cp _o/$1 "$OUTDIR"
  popd
}

export OUTDIR="$PWD/build"

# we use uasm - this is masm for unix :)
echo "Downloading uasm257_linux64.zip"
wget -q "https://github.com/Terraspace/UASM/releases/download/v2.57r/uasm257_linux64.zip"
unzip uasm257_linux64.zip uasm
rm -f uasm257_linux64.zip
sudo install -m 755 uasm /usr/bin
mkdir -p "$OUTDIR"

build 7za   Bundles/Alone
build 7zz   Bundles/Alone2
build 7zr   Bundles/Alone7z
build 7z.so Bundles/Format7zF
build 7z    UI/Console
