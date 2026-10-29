#!/bin/sh
# Builds the game logic (src/jack/) and the save store for the PC against tools/stub/, and
# runs tools/test.cpp
set -e
cd "$(dirname "$0")/.."
out="${TMPDIR:-/tmp}/jackhammer-test"
g++ -std=c++17 -O1 -Wall -Wextra -I tools/stub -I ../../lib/dotcore/src -o "$out" tools/test.cpp src/jack/*.cpp ../../lib/dotcore/src/SlotStore.cpp
"$out"
