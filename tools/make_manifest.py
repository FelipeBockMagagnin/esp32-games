#!/usr/bin/env python3
"""Writes the ESP Web Tools manifest of one game, or the public games.json.

make_manifest.py <id> <commit> <date>   docs/firmware/<id>/manifest.json
make_manifest.py --launcher <commit> <date>   docs/firmware/launcher/version.json
make_manifest.py --games                docs/games.js, the list with each game's version, and
                                        each game's icon for the launcher

The four images go at their own offsets rather than merged into one: merge_bin pads the gap
between the partition table (0x8000) and boot_app0 (0xE000) with 0xFF, which is where NVS
lives, so a merged image would erase every game's save.
"""
import json
import sys
from pathlib import Path

from PIL import Image

ROOT = Path(__file__).resolve().parent.parent
DOCS = ROOT / "docs"

PARTS = [
    ("bootloader.bin", 0x1000),
    ("partitions.bin", 0x8000),
    ("boot_app0.bin", 0xE000),
    ("firmware.bin", 0x10000),
]


# The launcher's 64x64 icon of each game, RGB565 high byte first (esp32-launcher's Catalog.h):
# a crop of its screen at `icon` [x, y, factor], 64 * factor pixels wide. Only a whole factor
# keeps the pixel art sharp, and the board draws it at exactly twice the size.
ICON_SIZE = 64
DEFAULT_ICON = [56, 96, 2]


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


def write_launcher(commit, date):
    out = DOCS / "firmware" / "launcher"
    version = {"version": commit, "date": date}
    (out / "version.json").write_text(json.dumps(version, indent=2) + "\n")
    print(f"launcher: {commit} ({date}), {(out / 'firmware.bin').stat().st_size / 1024:.0f} KB")


def write_thumb(game):
    x, y, factor = game.get("icon", DEFAULT_ICON)
    side = ICON_SIZE * factor
    image = Image.open(DOCS / game["image"]).convert("RGB").crop((x, y, x + side, y + side)).reduce(factor)
    rgb = image.tobytes()
    data = bytearray()
    for i in range(0, len(rgb), 3):
        c = (rgb[i] >> 3) << 11 | (rgb[i + 1] >> 2) << 5 | rgb[i + 2] >> 3
        data += bytes((c >> 8, c & 0xFF))
    (DOCS / "firmware" / game["id"] / "thumb.bin").write_bytes(data)


def write_games():
    public = []
    for g in games():
        entry = {k: v for k, v in g.items() if k not in ("path", "env")}
        entry["icon"] = g.get("icon", DEFAULT_ICON)
        manifest = DOCS / "firmware" / g["id"] / "manifest.json"
        if manifest.exists():
            m = json.loads(manifest.read_text())
            entry["version"] = m["version"]
            entry["date"] = m["date"]
            entry["manifest"] = f"firmware/{g['id']}/manifest.json"
            entry["size"] = (manifest.parent / "firmware.bin").stat().st_size
            write_thumb(g)
        public.append(entry)
    launcher = DOCS / "firmware" / "launcher"
    menu = None
    if (launcher / "version.json").exists():
        menu = json.loads((launcher / "version.json").read_text())
        menu["size"] = (launcher / "firmware.bin").stat().st_size
    # A script rather than JSON to fetch, so the list also loads from index.html opened as a file
    body = json.dumps(public, indent=2, ensure_ascii=False)
    (DOCS / "games.js").write_text(f"window.GAMES = {body};\nwindow.LAUNCHER = {json.dumps(menu)};\n")


if __name__ == "__main__":
    if sys.argv[1:] == ["--games"]:
        write_games()
    elif sys.argv[1] == "--launcher":
        write_launcher(*sys.argv[2:4])
    else:
        write_manifest(*sys.argv[1:4])
