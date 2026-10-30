#!/bin/sh
# Balance simulator over the real src/nan/: tools/sim.sh [--hours 48] [--every 60] [--seed 1]
set -e
cd "$(dirname "$0")/.."
TMPDIR=${TMPDIR:-/tmp}
g++ -std=c++17 -O2 -Wall -Wextra -o "$TMPDIR/nanidle-sim" tools/sim.cpp src/nan/Game.cpp src/nan/Format.cpp
"$TMPDIR/nanidle-sim" "$@"
