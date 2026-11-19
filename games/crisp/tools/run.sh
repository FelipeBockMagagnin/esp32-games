#!/bin/sh
# Builds tools/run.cpp with every game, under AddressSanitizer, and plays one (or draws the list or
# the settings; tools/run.cpp has the arguments):
#   tools/run.sh <game index or slug> <out.png> [keys:ms]...
# CRISP_LIBRARY and CRISP_CARTS build another catalogue instead of library.json and carts/, to try
# games before they join.
set -e
cd "$(dirname "$0")/.."
tft=$(ls -d .pio/libdeps/*/TFT_eSPI 2>/dev/null | head -1)
[ -n "$tft" ] || { ~/.platformio/penv/bin/pio pkg install > /dev/null; tft=$(ls -d .pio/libdeps/*/TFT_eSPI | head -1); }
library="${CRISP_LIBRARY:-library.json}"
carts="${CRISP_CARTS:-carts}"
work="${TMPDIR:-/tmp}/crisp-run"
objs="$work/objs-$(printf '%s' "$PWD/$library $carts" | md5sum | cut -c1-8)"
mkdir -p "$objs"
python3 tools/carts.py "$objs" "$library" "$carts"
common="-O1 -g -fsanitize=address,undefined -fno-sanitize=shift -DCRISP_PC=1 -DMAX_GAME_COUNT=300 -I lib/cglp/src -I src -I $objs"
cflags="-std=gnu11 -w -fwrapv $common"
cxxflags="-std=gnu++17 -w $common -DARDUINO=10819 -DTFT_WIDTH=240 -DTFT_HEIGHT=320
    -I tools/stub -I tools -I src/ui -I ../../lib/dotcore/src -I ../../lib/dotui/src -I $tft"
if [ "$(cat "$objs/flags" 2>/dev/null)" != "$cflags $cxxflags" ]; then
    rm -f "$objs"/*.o "$objs"/*.d
    printf '%s' "$cflags $cxxflags" > "$objs/flags"
fi
# Each source compiles once until it changes
linked=""
build() {
    o="$objs/$(echo "$2" | tr '/.' '__').o"
    linked="$linked $o"
    if [ ! -f "$o" ] || [ ! -f "$o.d" ] || [ -n "$(find $(sed 's/^[^:]*://; s/\\$//' "$o.d") -newer "$o" 2>/dev/null | head -1)" ]; then
        $1 -MMD -MF "$o.d" -c -o "$o" "$2" || { rm -f "$o"; touch "$objs/failed"; } &
    fi
}
for f in lib/cglp/src/*.c src/play/*.c $(python3 -c "import json,sys; print(' '.join('$carts/'+g['slug']+'.c' for g in json.load(open('$library'))))"); do
    build "gcc $cflags" "$f"
done
for f in src/play/Host.cpp src/ui/Theme.cpp src/screens/*.cpp src/core/ScreenManager.cpp ../../lib/dotcore/src/Settings.cpp \
    ../../lib/dotui/src/Box.cpp ../../lib/dotui/src/ButtonBadge.cpp ../../lib/dotui/src/Header.cpp \
    ../../lib/dotui/src/ListRow.cpp ../../lib/dotui/src/Text.cpp tools/run.cpp; do
    build "g++ $cxxflags" "$f"
done
wait
if [ -f "$objs/failed" ]; then
    rm -f "$objs/failed"
    exit 1
fi
g++ -fsanitize=address,undefined -o "$objs/run" $linked -lz
ASAN_OPTIONS=detect_leaks=0 UBSAN_OPTIONS="halt_on_error=1:print_stacktrace=1" "$objs/run" "$@"
