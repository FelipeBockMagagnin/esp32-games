#!/bin/sh
# Builds tools/sim.cpp and runs it: sim.sh [lives] [seed]
set -e
cd "$(dirname "$0")/.."
out="${TMPDIR:-/tmp}/boba-sim"
g++ -std=c++17 -O2 -Wall -Wextra -DPET_TOOLS=1 -I tools/stub -o "$out" tools/sim.cpp src/pet/Pet.cpp
"$out" "$@"
