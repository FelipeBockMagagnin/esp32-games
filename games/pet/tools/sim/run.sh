#!/usr/bin/env bash
# Builds the balance simulator against the real game logic and runs it.
# Usage: tools/sim/run.sh [--days 2] [--seed 1] [--session 5] [--log]
set -euo pipefail
ROOT="$(cd "$(dirname "$0")/../.." && pwd)"
OUT="${TMPDIR:-/tmp}/pet-sim"
g++ -std=c++17 -O2 -Wall -Wextra -Wno-missing-field-initializers -I"$ROOT/tools/sim/stub" -o "$OUT" \
    "$ROOT/tools/sim/sim.cpp" "$ROOT"/src/game/World.cpp "$ROOT"/src/game/Colony.cpp
"$OUT" "$@"
