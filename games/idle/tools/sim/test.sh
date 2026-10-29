#!/usr/bin/env bash
# Builds and runs the logic tests (tools/sim/test.cpp) against the real src/game/.
set -euo pipefail
ROOT="$(cd "$(dirname "$0")/../.." && pwd)"
OUT="${TMPDIR:-/tmp}/idle-test"
g++ -std=c++17 -O1 -Wall -Wextra -Wno-missing-field-initializers -Wno-unused-parameter -I"$ROOT/tools/sim/stub" -o "$OUT" \
    "$ROOT/tools/sim/test.cpp" "$ROOT"/src/game/*.cpp "$ROOT"/src/assets/Assets.cpp "$ROOT"/src/assets/Sprites.cpp
"$OUT"
