#!/usr/bin/env python3
"""Downloads the ROMs that library.json lists from the Homebrew Hub's database into roms/.

    tools/fetch_roms.py           only the ROMs missing from roms/
    tools/fetch_roms.py --all     every ROM again

Refuses a ROM the emulator cannot run: a Game Boy Color-only header (0xC0) or a cartridge
type Peanut-GB does not map. Writes each ROM's size and save size back into library.json, and
roms/LICENSES.md, the credit every game's licence asks for.
"""
import json
import sys
import urllib.parse
import urllib.request
from pathlib import Path

HERE = Path(__file__).resolve().parent.parent
LIBRARY = HERE / "library.json"
ROMS = HERE / "roms"
DATABASE = "https://raw.githubusercontent.com/gbdev/database/master/entries"

# Peanut-GB's cart_mbc[] table: the cartridge types it can map
SUPPORTED = {0x00, 0x01, 0x02, 0x03, 0x05, 0x06, 0x08, 0x09, 0x0B, 0x0C, 0x0D, 0x0F, 0x10, 0x11, 0x12, 0x13,
             0x19, 0x1A, 0x1B, 0x1C, 0x1D, 0x1E}
BATTERY = {0x03, 0x06, 0x09, 0x0D, 0x0F, 0x10, 0x13, 0x1B, 0x1E}
RAM_SIZES = [0, 2048, 8192, 32768, 131072, 65536]


def check(rom):
    if len(rom) < 0x150:
        return "too short"
    if rom[0x143] == 0xC0:
        return "Game Boy Color only"
    if rom[0x147] not in SUPPORTED:
        return f"cartridge type {rom[0x147]:#04x}"
    return None


def save_size(rom):
    kind = rom[0x147]
    if kind not in BATTERY:
        return 0
    if kind in (0x05, 0x06):
        return 512
    return RAM_SIZES[rom[0x149]] if rom[0x149] < len(RAM_SIZES) else 0


def write_credits(library):
    lines = ["# The games in roms/", "",
             "Each ROM is its author's work, redistributed unchanged under the licence they published it",
             "with, as listed on the Homebrew Hub (https://hh.gbdev.io). Its code, and for most its art,",
             "are at the source link.", ""]
    for g in sorted(library, key=lambda g: g["title"].lower()):
        lines.append(f"- **{g['title']}** by {g['author']}: {g['license']}. {g['source']}")
    (ROMS / "LICENSES.md").write_text("\n".join(lines) + "\n")


def main(args):
    library = json.loads(LIBRARY.read_text())
    ROMS.mkdir(exist_ok=True)
    failed = []
    for game in library:
        path = ROMS / f"{game['slug']}.gb"
        if not path.exists() or "--all" in args:
            url = f"{DATABASE}/{game['slug']}/{urllib.parse.quote(game['file'])}"
            print(f"{game['slug']}: {url}", flush=True)
            path.write_bytes(urllib.request.urlopen(url, timeout=60).read())
        rom = path.read_bytes()
        problem = check(rom)
        if problem:
            failed.append(f"{game['slug']}: {problem}")
            continue
        game["size"] = len(rom)
        game["save"] = save_size(rom)
    LIBRARY.write_text(json.dumps(library, indent=2, ensure_ascii=False) + "\n")
    write_credits(library)
    lite = [g for g in library if g["lite"]]
    print(f"{len(library)} games, {sum(g.get('size', 0) for g in library) // 1024} KB; "
          f"lite: {len(lite)} games, {sum(g.get('size', 0) for g in lite) // 1024} KB")
    if failed:
        sys.exit("Refused:\n" + "\n".join(failed))


if __name__ == "__main__":
    main(sys.argv[1:])
