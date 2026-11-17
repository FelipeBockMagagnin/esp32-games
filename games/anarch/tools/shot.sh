#!/bin/sh
# Builds the game for the PC with the firmware's settings and saves a frame of level 1:
#   tools/shot.sh <out.png> [key:frames]...   (see tools/shot.cpp)
set -e
cd "$(dirname "$0")/.."
bin="${TMPDIR:-/tmp}/anarch-shot"
g++ -std=c++17 -O2 -w -I src -o "$bin" tools/shot.cpp -lz
"$bin" "$@"
