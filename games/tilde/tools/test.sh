#!/bin/sh
# Builds the pet's logic, the drawing and the save store for the PC against tools/stub/, and
# runs tools/test.cpp
set -e
cd "$(dirname "$0")/.."
out="${TMPDIR:-/tmp}/tilde-test"
g++ -std=c++17 -O1 -Wall -Wextra -DPET_TOOLS -I tools/stub -I ../../lib/dotcore/src -o "$out" tools/test.cpp src/pet/*.cpp src/art/Rig.cpp \
    src/gfx/Canvas.cpp src/gfx/Icons.cpp src/gfx/FontData.cpp src/gfx/Theme.cpp ../../lib/dotcore/src/SlotStore.cpp
"$out"
