#!/usr/bin/env bash
# Builds and runs the difficulty ladder (tools/sim/ladder.cpp) against the real src/game/.
# Usage: tools/sim/ladder.sh [upgrade price factor, default 4] [minutes per fight, default 60]
set -euo pipefail
ROOT="$(cd "$(dirname "$0")/../.." && pwd)"
OUT="${TMPDIR:-/tmp}/idle-ladder"
g++ -std=c++17 -O2 -Wall -Wextra -Wno-missing-field-initializers -Wno-unused-parameter -I"$ROOT/tools/sim/stub" -o "$OUT" \
    "$ROOT/tools/sim/ladder.cpp" "$ROOT"/src/game/*.cpp "$ROOT"/src/assets/Assets.cpp "$ROOT"/src/assets/Sprites.cpp
"$OUT" "$@"
