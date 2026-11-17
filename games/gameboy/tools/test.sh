#!/bin/sh
# Builds the emulator's logic (src/emu/) for the PC against tools/stub/ and runs tools/test.cpp,
# which also boots every ROM in library.json
set -e
cd "$(dirname "$0")/.."
out="${TMPDIR:-/tmp}/gameboy-test"
mkdir -p "$out"
gcc -std=c99 -O2 -c -o "$out/core.o" src/emu/GbCore.c
g++ -std=c++17 -O2 -Wall -Wextra -I tools/stub -o "$out/test" tools/test.cpp \
    src/emu/Apu.cpp src/emu/GameBoy.cpp src/emu/Pack.cpp src/emu/SaveCodec.cpp src/emu/Saves.cpp "$out/core.o"
"$out/test"
