#!/usr/bin/env bash
# Films a floor with the game's renderer: the whole map in one strip, or the view after the
# solver's route has played for a number of steps. Writes a PNG.
#   tools/sim/film.sh map [level] [out.png]
#   tools/sim/film.sh frame <level> <steps> [out.png]
set -euo pipefail
ROOT="$(cd "$(dirname "$0")/../.." && pwd)"
BIN="${TMPDIR:-/tmp}/decaf-film"
g++ -std=c++17 -O2 -Wall -Wextra -Wno-missing-field-initializers -Wno-unused-parameter -I"$ROOT/tools/sim/stub" -o "$BIN" \
    "$ROOT/tools/sim/film.cpp" "$ROOT"/src/game/Stage.cpp "$ROOT"/src/render/Renderer.cpp "$ROOT"/src/assets/Sprites.cpp
mode="${1:-map}"
level="${2:-0}"
if [ "$mode" = map ]; then
    out="${3:-map.png}"
    args=(map "$level" "$out.ppm")
else
    out="${4:-frame.png}"
    args=(frame "$level" "$3" "$out.ppm")
fi
"$BIN" "${args[@]}"
python3 -c 'import sys; from PIL import Image; Image.open(sys.argv[1]).save(sys.argv[2])' "$out.ppm" "$out"
rm "$out.ppm"
echo "$out"
