#!/bin/sh
# Builds the screens for the PC against tools/stub/ and saves their scenes as PNGs into the given
# folder (default: $TMPDIR/dice-shots). The fonts come from the TFT_eSPI that PlatformIO fetches,
# so the firmware is built first if it never was.
set -e
cd "$(dirname "$0")/.."
out="${1:-${TMPDIR:-/tmp}/dice-shots}"
tft=.pio/libdeps/esp32dev/TFT_eSPI
[ -d "$tft" ] || ~/.platformio/penv/bin/pio run > /dev/null
rm -rf "$out"
mkdir -p "$out"
bin="${TMPDIR:-/tmp}/dice-shots-bin"
g++ -std=c++17 -O2 -Wall -Wextra -Wno-unused-parameter -I tools/stub -I "$tft" -I src/ui -I ../../lib/dotcore/src -I ../../lib/dotui/src \
    -o "$bin" tools/shots.cpp src/dice/*.cpp src/ui/*.cpp src/screens/*.cpp src/core/ScreenManager.cpp \
    ../../lib/dotcore/src/SlotStore.cpp ../../lib/dotcore/src/Settings.cpp \
    ../../lib/dotui/src/Box.cpp ../../lib/dotui/src/Text.cpp ../../lib/dotui/src/ListRow.cpp \
    ../../lib/dotui/src/Header.cpp ../../lib/dotui/src/ButtonBadge.cpp -lz
"$bin" "$out"
ls "$out"
