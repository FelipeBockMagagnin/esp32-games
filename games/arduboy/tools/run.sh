#!/bin/sh
# Builds tools/run.cpp with every game, under AddressSanitizer, and plays one (or draws the list or
# the settings; tools/run.cpp has the arguments):
#   tools/run.sh <game index or slug> <out.png> [keys:ms]...
# The firmware build runs first, for the games and the table tools/sketches.py writes.
set -e
cd "$(dirname "$0")/.."
~/.platformio/penv/bin/pio run > /dev/null
tft=.pio/libdeps/esp32dev/TFT_eSPI
bin="${TMPDIR:-/tmp}/arduboy-run"
objs="${TMPDIR:-/tmp}/arduboy-run-objs"
mkdir -p "$objs"
flags="-std=gnu++17 -O1 -g -fsanitize=address,undefined -fno-sanitize=shift -w -fpermissive -Wno-narrowing
    -DARDUINO=10819 -DDISPLAY_LANDSCAPE=1 -DTFT_WIDTH=240 -DTFT_HEIGHT=320 -DAB_PC=1
    -I tools/stub -I tools -I lib/AvrShim/src -I lib/Arduboy2/src -I lib/ArduboyTones/src -I lib/FixedPoints/src -I src -I src/ui -I .pio/build/esp32dev/sketches
    -I ../../lib/dotcore/src -I ../../lib/dotui/src -I $tft"
# New flags rebuild everything
if [ "$(cat "$objs/flags" 2>/dev/null)" != "$flags" ]; then
    rm -f "$objs"/*.o "$objs"/*.d
    printf '%s' "$flags" > "$objs/flags"
fi
# Each source compiles once until it changes; the games are the slow part
linked=""
for f in lib/Arduboy2/src/*.cpp lib/ArduboyTones/src/*.cpp src/play/*.cpp src/save/*.cpp src/ui/Theme.cpp \
    src/screens/*.cpp src/core/ScreenManager.cpp .pio/build/esp32dev/sketches/*.cpp ../../lib/dotcore/src/Settings.cpp \
    ../../lib/dotui/src/Box.cpp ../../lib/dotui/src/ButtonBadge.cpp ../../lib/dotui/src/Header.cpp \
    ../../lib/dotui/src/ListRow.cpp ../../lib/dotui/src/Text.cpp tools/run.cpp; do
    o="$objs/$(echo "$f" | tr '/.' '__').o"
    linked="$linked $o"
    if [ ! -f "$o" ] || [ ! -f "$o.d" ] || [ -n "$(find $(sed 's/^[^:]*://; s/\\$//' "$o.d") -newer "$o" 2>/dev/null | head -1)" ]; then
        g++ $flags -MMD -MF "$o.d" -c -o "$o" "$f" || { rm -f "$o"; touch "$objs/failed"; } &
    fi
done
wait
if [ -f "$objs/failed" ]; then
    rm -f "$objs/failed"
    exit 1
fi
g++ -fsanitize=address,undefined -o "$bin" $linked -lz
# Recoverable, so tools/ubsan.supp can let a known report pass; any other still stops the run
UBSAN_OPTIONS="halt_on_error=1:print_stacktrace=1:suppressions=$PWD/tools/ubsan.supp" "$bin" "$@"
