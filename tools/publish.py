#!/usr/bin/env python3
"""Builds the games in games/ and the launcher, and writes the site's firmware and game list.

    tools/publish.py            every game whose firmware changed since it was last published
    tools/publish.py idle pet   just these (and `launcher` for the menu)
    tools/publish.py --all      everything, changed or not
    tools/publish.py --list     only rewrite docs/games.js and the screens, building nothing

A game is a folder games/<id>/ with a game.json (name, tagline, category, description, accent,
and optional icon, order, env) and a screen.png; the folder name is its id. A game's version is
the last commit that touched its firmware's sources, so a change to its description or to
another game rebuilds nothing. Commits nothing.

The four images go at their own offsets rather than merged into one: merge_bin pads the gap
between the partition table (0x8000) and boot_app0 (0xE000) with 0xFF, which is where NVS
lives, so a merged image would erase every game's save.
"""
import json
import os
import shutil
import subprocess
import sys
from pathlib import Path

from PIL import Image

ROOT = Path(__file__).resolve().parent.parent
GAMES = ROOT / "games"
DOCS = ROOT / "docs"
FIRMWARE = DOCS / "firmware"
PIO = os.environ.get("PIO", str(Path.home() / ".platformio/penv/bin/pio"))
BOOT_APP0 = Path.home() / ".platformio/packages/framework-arduinoespressif32/tools/partitions/boot_app0.bin"
# Every firmware also builds these, so a change to one is a change to all
COMMON = ["lib", "common.ini"]
REQUIRED = ["name", "tagline", "category", "description", "accent"]

PARTS = [
    ("bootloader.bin", 0x1000),
    ("partitions.bin", 0x8000),
    ("boot_app0.bin", 0xE000),
    ("firmware.bin", 0x10000),
]

# The launcher's 64x64 icon of each game, RGB565 high byte first (launcher/src/Catalog.h):
# a crop of its screen at `icon` [x, y, factor], 64 * factor pixels wide. Only a whole factor
# keeps the pixel art sharp, and the board draws it at exactly twice the size.
ICON_SIZE = 64
DEFAULT_ICON = [56, 96, 2]


def git(*args):
    return subprocess.run(["git", "-C", str(ROOT), *args], check=True, capture_output=True, text=True).stdout.strip()


def load_categories():
    return json.loads((GAMES / "categories.json").read_text())


def load_games():
    shelves = {c["id"] for c in load_categories()}
    games = []
    for path in sorted(GAMES.glob("*/game.json")):
        game = json.loads(path.read_text())
        game["id"] = path.parent.name
        missing = [k for k in REQUIRED if k not in game]
        if missing:
            sys.exit(f"{path}: missing {', '.join(missing)}")
        if game["category"] not in shelves:
            sys.exit(f"{path}: category {game['category']!r} is not in games/categories.json")
        if not (path.parent / "screen.png").exists():
            sys.exit(f"{path.parent}: no screen.png")
        games.append(game)
    games.sort(key=lambda g: (g.get("order", 1000), g["id"]))
    return games


class Target:
    def __init__(self, tid, folder, env="esp32dev"):
        self.id = tid
        self.dir = ROOT / folder
        self.env = env
        self.sources = [f"{folder}/{p}" for p in ("src", "include", "lib", "platformio.ini")] + COMMON
        self.out = FIRMWARE / tid

    def version(self):
        return git("log", "-1", "--format=%h %cs", "--", *self.sources).split()

    def published(self):
        meta = self.out / ("version.json" if self.id == "launcher" else "manifest.json")
        return json.loads(meta.read_text())["version"] if meta.exists() else None

    def refusal(self):
        # A published firmware must match a commit, so the version on the site means something
        if git("status", "--porcelain", "--", *self.sources):
            return "uncommitted changes; commit them first"
        # Nor build in a git-ignored source file, like a secrets.h with someone's WiFi password
        ignored = git("ls-files", "--others", "--ignored", "--exclude-standard", "--",
                      f"{self.dir.relative_to(ROOT)}/src", f"{self.dir.relative_to(ROOT)}/include")
        if ignored:
            return "ignored files would go into the firmware:\n" + ignored
        return None

    def build(self, game=None):
        commit, date = self.version()
        print(f"== {self.id} ({self.dir.relative_to(ROOT)}, env {self.env})", flush=True)
        # From clean: a cached object may predate the commit, or have seen a file deleted since
        subprocess.run([PIO, "run", "-d", str(self.dir), "-e", self.env, "-t", "clean"], check=True,
                       stdout=subprocess.DEVNULL)
        subprocess.run([PIO, "run", "-d", str(self.dir), "-e", self.env], check=True)
        build = self.dir / ".pio" / "build" / self.env
        self.out.mkdir(parents=True, exist_ok=True)
        if game is None:
            for name in ("bootloader.bin", "firmware.bin"):
                shutil.copy(build / name, self.out)
            write_json(self.out / "version.json", {"version": commit, "date": date})
        else:
            for name in ("bootloader.bin", "partitions.bin", "firmware.bin"):
                shutil.copy(build / name, self.out)
            shutil.copy(BOOT_APP0, self.out)
            write_json(self.out / "manifest.json", {
                "name": game["name"],
                "version": commit,
                "date": date,
                # Asks before erasing; the default keeps NVS, so other games' saves survive
                "new_install_prompt_erase": True,
                # The games speak no Improv Wi-Fi: don't wait for it after flashing
                "new_install_improv_wait_time": 0,
                "builds": [{"chipFamily": "ESP32", "parts": [{"path": p, "offset": o} for p, o in PARTS]}],
            })
        print(f"{self.id}: {commit} ({date}), {(self.out / 'firmware.bin').stat().st_size / 1024:.0f} KB")


def write_json(path, data):
    path.write_text(json.dumps(data, indent=2, ensure_ascii=False) + "\n")


def write_thumb(game, screen):
    x, y, factor = game.get("icon", DEFAULT_ICON)
    side = ICON_SIZE * factor
    image = Image.open(screen).convert("RGB").crop((x, y, x + side, y + side)).reduce(factor)
    rgb = image.tobytes()
    data = bytearray()
    for i in range(0, len(rgb), 3):
        c = (rgb[i] >> 3) << 11 | (rgb[i + 1] >> 2) << 5 | rgb[i + 2] >> 3
        data += bytes((c >> 8, c & 0xFF))
    (FIRMWARE / game["id"] / "thumb.bin").write_bytes(data)


def write_list(games):
    """docs/games.js, each game's screen and launcher icon, and nothing left of a removed game"""
    ids = {g["id"] for g in games}
    for folder in FIRMWARE.iterdir():
        if folder.is_dir() and folder.name != "launcher" and folder.name not in ids:
            shutil.rmtree(folder)
            print(f"{folder.name}: removed")
    for image in (DOCS / "img").glob("*.png"):
        if image.stem not in ids:
            image.unlink()

    public = []
    for g in games:
        screen = DOCS / "img" / f"{g['id']}.png"
        shutil.copy(GAMES / g["id"] / "screen.png", screen)
        entry = {"id": g["id"], **{k: v for k, v in g.items() if k not in ("id", "env", "order")}}
        entry["image"] = f"img/{g['id']}.png"
        entry["icon"] = g.get("icon", DEFAULT_ICON)
        manifest = FIRMWARE / g["id"] / "manifest.json"
        if manifest.exists():
            m = json.loads(manifest.read_text())
            entry["version"] = m["version"]
            entry["date"] = m["date"]
            entry["manifest"] = f"firmware/{g['id']}/manifest.json"
            entry["size"] = (manifest.parent / "firmware.bin").stat().st_size
            write_thumb(g, screen)
        public.append(entry)

    launcher = FIRMWARE / "launcher"
    menu = None
    if (launcher / "version.json").exists():
        menu = json.loads((launcher / "version.json").read_text())
        menu["size"] = (launcher / "firmware.bin").stat().st_size
    # A script rather than JSON to fetch, so the list also loads from index.html opened as a file
    body = json.dumps(public, indent=2, ensure_ascii=False)
    shelves = json.dumps(load_categories(), indent=2, ensure_ascii=False)
    (DOCS / "games.js").write_text(f"window.CATEGORIES = {shelves};\nwindow.GAMES = {body};\n"
                                   f"window.LAUNCHER = {json.dumps(menu)};\n")


def main(args):
    games = load_games()
    every = "--all" in args
    named = [a for a in args if not a.startswith("--")]
    by_id = {g["id"]: g for g in games}
    unknown = [n for n in named if n not in by_id and n != "launcher"]
    if unknown:
        sys.exit(f"No game named {', '.join(unknown)} in games/")

    targets = [(Target(g["id"], f"games/{g['id']}", g.get("env", "esp32dev")), g) for g in games]
    targets.append((Target("launcher", "launcher"), None))
    if named:
        targets = [(t, g) for t, g in targets if t.id in named]

    built, refused = [], []
    if "--list" not in args:
        for target, game in targets:
            # Unasked for, a game is rebuilt only when its sources moved past the published commit
            if not named and not every and target.published() == target.version()[0]:
                continue
            reason = target.refusal()
            if reason:
                refused.append(target.id)
                print(f"{target.id}: {reason}", file=sys.stderr)
                continue
            target.build(game)
            built.append(target.id)

    write_list(games)
    print()
    print(f"Built: {', '.join(built) or 'nothing, all up to date'}")
    if refused:
        print(f"Refused: {', '.join(refused)}", file=sys.stderr)
    print(git("status", "--short", "docs") or "docs/ unchanged")
    print("\nTo publish: git add docs && git commit -m 'publish firmware' && git push")
    return 1 if refused else 0


if __name__ == "__main__":
    sys.exit(main(sys.argv[1:]))
