#!/bin/sh
# Builds the pet's logic for the PC and plays its first days with a few kinds of owner:
# tools/sim.sh [days] [seed]
set -e
cd "$(dirname "$0")/.."
out="${TMPDIR:-/tmp}/tilde-sim"
g++ -std=c++17 -O2 -Wall -Wextra -DPET_TOOLS -I tools/stub -o "$out" tools/sim.cpp src/pet/*.cpp src/core/SlotStore.cpp
"$out" "$@"
