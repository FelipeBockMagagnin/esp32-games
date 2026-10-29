#!/bin/sh
# Builds tools/frame.cpp (the game's renderer and art on the PC) and writes the arena as a PPM.
# Usage: tools/frame.sh [seconds] [out.ppm]
set -e
cd "$(dirname "$0")/.."
bin="${TMPDIR:-/tmp}/jackhammer-frame"
g++ -std=c++17 -O1 -Wall -Wextra -I tools/stub -o "$bin" tools/frame.cpp src/jack/Round.cpp src/render/*.cpp src/assets/Sprites.cpp
"$bin" "$@"
