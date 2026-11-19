#!/bin/sh
# Builds the simulations (src/fluid/) and the save store for the PC against tools/stub/, and runs
# tools/test.cpp
set -e
cd "$(dirname "$0")/.."
out="${TMPDIR:-/tmp}/fluid-test"
g++ -std=c++17 -O2 -Wall -Wextra -I tools/stub -I ../../lib/dotcore/src -o "$out" tools/test.cpp src/fluid/*.cpp ../../lib/dotcore/src/SlotStore.cpp
"$out"
