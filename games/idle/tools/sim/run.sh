#!/usr/bin/env bash
# Builds the balance simulator against the real game logic and runs it.
# Usage: tools/sim/run.sh [--days 3] [--seed 1] [--session 5] [--cps 3]
set -euo pipefail
ROOT="$(cd "$(dirname "$0")/../.." && pwd)"
OUT="${TMPDIR:-/tmp}/idle-sim"
g++ -std=c++17 -O2 -w -I"$ROOT/tools/sim/stub" -o "$OUT" \
    "$ROOT/tools/sim/sim.cpp" \
    "$ROOT"/src/game/GameState.cpp "$ROOT"/src/game/Inventory.cpp "$ROOT"/src/game/CombatState.cpp \
    "$ROOT"/src/game/Achievements.cpp "$ROOT"/src/game/Stats.cpp \
    "$ROOT"/src/assets/Assets.cpp "$ROOT"/src/assets/Sprites.cpp
"$OUT" "$@"
