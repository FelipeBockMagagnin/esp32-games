#!/usr/bin/env bash
# Builds the games listed in games.json and puts their firmware under docs/, for GitHub
# Pages. Usage: ./publish.sh [id...]   (no id: every game). Commits nothing.
set -euo pipefail
ROOT="$(cd "$(dirname "$0")" && pwd)"
PIO="${PIO:-$HOME/.platformio/penv/bin/pio}"
BOOT_APP0="$HOME/.platformio/packages/framework-arduinoespressif32/tools/partitions/boot_app0.bin"

mapfile -t GAMES < <(python3 -c '
import json, sys
games = json.load(open(sys.argv[1]))
wanted = sys.argv[2:]
for g in games:
    if not wanted or g["id"] in wanted:
        print(g["id"], g["path"], g["env"])
' "$ROOT/games.json" "$@")

if [ "${#GAMES[@]}" -eq 0 ]; then
    echo "No game matches: $*" >&2
    exit 1
fi

for line in "${GAMES[@]}"; do
    read -r id path env <<<"$line"
    src="$(cd "$ROOT" && cd "$path" && pwd)"
    # A published firmware must match a commit, so the version on the site means something
    if [ -n "$(git -C "$src" status --porcelain)" ]; then
        echo "$id: $src has uncommitted changes; commit them first" >&2
        exit 1
    fi
    echo "== $id ($src, env $env)"
    "$PIO" run -d "$src" -e "$env"
    out="$ROOT/docs/firmware/$id"
    mkdir -p "$out"
    build="$src/.pio/build/$env"
    cp "$build/bootloader.bin" "$build/partitions.bin" "$build/firmware.bin" "$out/"
    cp "$BOOT_APP0" "$out/"
    python3 "$ROOT/tools/make_manifest.py" "$id" "$(git -C "$src" rev-parse --short HEAD)" \
        "$(git -C "$src" log -1 --format=%cs)"
done

python3 "$ROOT/tools/make_manifest.py" --games
git -C "$ROOT" status --short docs
echo
echo "To publish: git add docs && git commit -m 'publish firmware' && git push"
