#!/bin/sh
# Builds the logic (src/pet/) and the save store for the PC against tools/stub/, and runs
# tools/test.cpp
set -e
cd "$(dirname "$0")/.."
out="${TMPDIR:-/tmp}/boba-test"
g++ -std=c++17 -O1 -Wall -Wextra -DPET_TOOLS=1 -I tools/stub -I ../../lib/dotcore/src -o "$out" tools/test.cpp src/pet/*.cpp ../../lib/dotcore/src/SlotStore.cpp
"$out"
