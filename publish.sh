#!/usr/bin/env bash
# Builds the games listed in games.json, and the menu that boots them when several share a
# board (../esp32-launcher, id `launcher`), and puts their firmware under docs/, for GitHub
# Pages. Usage: ./publish.sh [id...]   (no id: every game and the launcher). Commits nothing.
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
if not wanted or "launcher" in wanted:
    print("launcher", "../esp32-launcher", "esp32dev")
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
    # Nor build in a git-ignored source file, like a secrets.h with someone's WiFi password
    ignored="$(git -C "$src" ls-files --others --ignored --exclude-standard -- src include)"
    if [ -n "$ignored" ]; then
        echo "$id: ignored files would go into the firmware:" >&2
        echo "$ignored" >&2
        exit 1
    fi
    echo "== $id ($src, env $env)"
    # From clean: a cached object may predate the commit, or have seen a file deleted since
    "$PIO" run -d "$src" -e "$env" -t clean >/dev/null
    "$PIO" run -d "$src" -e "$env"
    out="$ROOT/docs/firmware/$id"
    mkdir -p "$out"
    build="$src/.pio/build/$env"
    commit="$(git -C "$src" rev-parse --short HEAD)"
    date="$(git -C "$src" log -1 --format=%cs)"
    if [ "$id" = launcher ]; then
        cp "$build/bootloader.bin" "$build/firmware.bin" "$out/"
        python3 "$ROOT/tools/make_manifest.py" --launcher "$commit" "$date"
        continue
    fi
    cp "$build/bootloader.bin" "$build/partitions.bin" "$build/firmware.bin" "$out/"
    cp "$BOOT_APP0" "$out/"
    python3 "$ROOT/tools/make_manifest.py" "$id" "$commit" "$date"
done

python3 "$ROOT/tools/make_manifest.py" --games
git -C "$ROOT" status --short docs
echo
echo "To publish: git add docs && git commit -m 'publish firmware' && git push"
