#!/bin/sh
# Builds the screens and the player for the PC against tools/stub/ and saves their scenes as PNGs:
#   tools/shots.sh [lite|max] [out_dir] [theme] [game index[:frames]]...
# The pack and the fonts come from the firmware build, which runs first if it never did.
set -e
cd "$(dirname "$0")/.."
tier="${1:-lite}"
out="${2:-${TMPDIR:-/tmp}/gameboy-shots}"
[ $# -gt 0 ] && shift
[ $# -gt 0 ] && shift
project=.
[ "$tier" = max ] && project=../gameboymax
tft=.pio/libdeps/esp32dev/TFT_eSPI
pack="$project/.pio/build/esp32dev/gbpack.bin"
[ -d "$tft" ] || ~/.platformio/penv/bin/pio run > /dev/null
[ -f "$pack" ] || ~/.platformio/penv/bin/pio run -d "$project" > /dev/null
rm -rf "$out"
mkdir -p "$out"
bin="${TMPDIR:-/tmp}/gameboy-shots-bin"
gcc -std=c99 -O2 -c -o "$bin-core.o" src/emu/GbCore.c
g++ -std=c++17 -O2 -Wall -Wextra -Wno-unused-parameter -I tools/stub -I "$tft" -I src/ui -I ../../lib/dotcore/src -I ../../lib/dotui/src \
    -o "$bin" tools/shots.cpp src/ui/*.cpp src/screens/*.cpp src/core/ScreenManager.cpp src/play/Player.cpp \
    src/emu/Apu.cpp src/emu/GameBoy.cpp src/emu/Pack.cpp src/emu/SaveCodec.cpp src/emu/Saves.cpp "$bin-core.o" \
    ../../lib/dotcore/src/Settings.cpp \
    ../../lib/dotui/src/Box.cpp ../../lib/dotui/src/Text.cpp ../../lib/dotui/src/ListRow.cpp \
    ../../lib/dotui/src/Header.cpp ../../lib/dotui/src/ButtonBadge.cpp -lz
"$bin" "$pack" "$out" "$@"
ls "$out"
