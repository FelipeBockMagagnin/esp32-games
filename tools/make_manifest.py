#!/usr/bin/env python3
"""Writes the ESP Web Tools manifest of one game, or the public games.json.

make_manifest.py <id> <commit> <date>   docs/firmware/<id>/manifest.json
make_manifest.py --games                docs/games.js, the list with each game's version

The four images go at their own offsets rather than merged into one: merge_bin pads the gap
between the partition table (0x8000) and boot_app0 (0xE000) with 0xFF, which is where NVS
lives, so a merged image would erase every game's save.
"""
import json
import sys
from pathlib import Path

ROOT = Path(__file__).resolve().parent.parent
DOCS = ROOT / "docs"

PARTS = [
    ("bootloader.bin", 0x1000),
    ("partitions.bin", 0x8000),
    ("boot_app0.bin", 0xE000),
    ("firmware.bin", 0x10000),
]


def games():
    return json.loads((ROOT / "games.json").read_text())


def write_manifest(game_id, commit, date):
    game = next(g for g in games() if g["id"] == game_id)
    out = DOCS / "firmware" / game_id
    manifest = {
        "name": game["name"],
        "version": commit,
        "date": date,
        # Asks before erasing; the default keeps NVS, so other games' saves survive
        "new_install_prompt_erase": True,
        # The games speak no Improv Wi-Fi: don't wait for it after flashing
        "new_install_improv_wait_time": 0,
        "builds": [{"chipFamily": "ESP32", "parts": [{"path": p, "offset": o} for p, o in PARTS]}],
    }
    (out / "manifest.json").write_text(json.dumps(manifest, indent=2) + "\n")
    size = sum((out / p).stat().st_size for p, _ in PARTS)
    print(f"{game_id}: {commit} ({date}), {size / 1024:.0f} KB")


def write_games():
    public = []
    for g in games():
        entry = {k: v for k, v in g.items() if k not in ("path", "env")}
        manifest = DOCS / "firmware" / g["id"] / "manifest.json"
        if manifest.exists():
            m = json.loads(manifest.read_text())
            entry["version"] = m["version"]
            entry["date"] = m["date"]
            entry["manifest"] = f"firmware/{g['id']}/manifest.json"
        public.append(entry)
    # A script rather than JSON to fetch, so the list also loads from index.html opened as a file
    body = json.dumps(public, indent=2, ensure_ascii=False)
    (DOCS / "games.js").write_text(f"window.GAMES = {body};\n")


if __name__ == "__main__":
    if sys.argv[1:] == ["--games"]:
        write_games()
    else:
        write_manifest(*sys.argv[1:4])
