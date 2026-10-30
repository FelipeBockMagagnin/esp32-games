#!/bin/sh
# Builds the screens for the PC against tools/stub/ and saves their scenes as PNGs into the given
# folder (default: $TMPDIR/nanidle-shots). The fonts come from the TFT_eSPI that PlatformIO
# fetches, so the firmware is built first if it never was.
set -e
cd "$(dirname "$0")/.."
out="${1:-${TMPDIR:-/tmp}/nanidle-shots}"
tft=.pio/libdeps/esp32dev/TFT_eSPI
[ -d "$tft" ] || ~/.platformio/penv/bin/pio run > /dev/null
rm -rf "$out"
mkdir -p "$out"
bin="${TMPDIR:-/tmp}/nanidle-shots-bin"
g++ -std=c++17 -O2 -Wall -Wextra -Wno-unused-parameter -I tools/stub -I "$tft" -I src/ui -I ../../lib/dotcore/src -I ../../lib/dotui/src \
    -o "$bin" tools/shots.cpp src/nan/*.cpp src/ui/*.cpp src/screens/*.cpp src/assets/Sprites.cpp src/core/ScreenManager.cpp \
    ../../lib/dotcore/src/SlotStore.cpp ../../lib/dotcore/src/Settings.cpp ../../lib/dotui/src/Text.cpp -lz
"$bin" "$out"
ls "$out"
