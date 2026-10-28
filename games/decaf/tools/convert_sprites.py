#!/usr/bin/env python3
"""Converts the artist's PNGs in src/assets/png/sprites/ into src/assets/Sprites.{h,cpp}.

Sheets (sheets/*.png, frames side by side, sizes in SHEETS) become a SheetDef: one palette for
the sheet and, per frame, the opaque box inside the frame as one palette index per pixel,
index 0 being transparent. Indexed rather than run-length encoded, so the band renderer can
start a frame at any row and flip it.

Tiles are sliced from tiles/tileset.png at the spots in TILES into 16x16 RGB565 blocks, black
transparent like the icons in the other games.

Splash art (splash/*.png) keeps the run-length SpriteDef the engine's SplashArt draws.

Colours are byte-swapped, because pushImage and the DMA band push send words raw.

Run from the repo root: python3 tools/convert_sprites.py
"""
import sys
from pathlib import Path

from PIL import Image

ROOT = Path(__file__).resolve().parent.parent
SRC = ROOT / "src" / "assets" / "png" / "sprites"
OUT_H = ROOT / "src" / "assets" / "Sprites.h"
OUT_CPP = ROOT / "src" / "assets" / "Sprites.cpp"

ALPHA_CUTOFF = 128
NEAR_BLACK = 0x0020  # Opaque black in a tile, since 0x0000 is the transparent key
TILE = 16

SHEETS = {
    "player": (32, 27),
    "walker": (32, 32),
    "drunk": (32, 32),
    "printer": (32, 32),
    "paper": (8, 8),
    "boss1": (64, 64),
    "crumble": (48, 16),
    "crumble_small": (16, 16),
    "woman": (32, 32),
    "coffee": (32, 32),
}

# Top-left corner of each 16x16 tile in tileset.png, in TileArt order
TILES = [
    ("GROUND_TL", 0, 0), ("GROUND_T0", 16, 0), ("GROUND_T1", 48, 0), ("GROUND_T2", 80, 0), ("GROUND_T3", 112, 0),
    ("GROUND_TR", 160, 0),
    ("GROUND_L", 0, 16), ("GROUND_M0", 16, 16), ("GROUND_M1", 64, 16), ("GROUND_R", 160, 16),
    ("GROUND_BL", 0, 32), ("GROUND_B", 16, 32), ("GROUND_BR", 160, 32),
    ("BLOCK", 192, 0),
    ("SPIKE_UP", 240, 32), ("SPIKE_DOWN", 240, 0),
    ("DESK_L", 286, 300), ("DESK_M", 302, 300), ("DESK_R", 318, 300),
    ("CABINET_TL", 236, 300), ("CABINET_T", 252, 300), ("CABINET_TR", 268, 300),
    ("CABINET_L", 236, 316), ("CABINET_M", 252, 316), ("CABINET_R", 268, 316),
    ("CABINET_BL", 236, 332), ("CABINET_B", 252, 332), ("CABINET_BR", 268, 332),
    ("POST", 350, 300),
]


def rgb565(r, g, b):
    return ((r >> 3) << 11) | ((g >> 2) << 5) | (b >> 3)


def swap(c):
    return ((c >> 8) | (c << 8)) & 0xFFFF


def words(values, per_line=16):
    return ",\n    ".join(", ".join(values[i:i + per_line]) for i in range(0, len(values), per_line))


def convert_sheet(name, path, frame_w, frame_h):
    im = Image.open(path).convert("RGBA")
    if im.height != frame_h or im.width % frame_w:
        sys.exit(f"{path}: {im.size} is not a row of {frame_w}x{frame_h} frames")
    count = im.width // frame_w

    palette = []
    lookup = {}
    body = []
    frames = []
    cost = 0
    for i in range(count):
        frame = im.crop((i * frame_w, 0, (i + 1) * frame_w, frame_h))
        alpha = frame.getchannel("A").point(lambda a: 255 if a >= ALPHA_CUTOFF else 0)
        box = alpha.getbbox()
        if box is None:
            frames.append("{0, 0, 0, 0, nullptr}")
            continue
        x0, y0, x1, y1 = box
        pixels = []
        for y in range(y0, y1):
            for x in range(x0, x1):
                r, g, b, a = frame.getpixel((x, y))
                if a < ALPHA_CUTOFF:
                    pixels.append(0)
                    continue
                c = rgb565(r, g, b)
                if c not in lookup:
                    palette.append(c)
                    lookup[c] = len(palette)
                pixels.append(lookup[c])
        body.append(f"static const uint8_t PROGMEM {name}_{i}[] = {{\n    {words([str(p) for p in pixels], 32)}}};")
        frames.append(f"{{{x0}, {y0}, {x1 - x0}, {y1 - y0}, {name}_{i}}}")
        cost += len(pixels)
    if len(palette) > 255:
        sys.exit(f"{path}: {len(palette)} colours, the format holds 255")

    body.append(f"static const uint16_t PROGMEM {name}_palette[] = {{\n    "
                f"{words([f'0x{swap(c):04X}' for c in palette], 12)}}};")
    body.append(f"static const FrameDef {name}_frames[] = {{\n    {', '.join(frames)}}};")
    body.append(f"const SheetDef {name} = {{{frame_w}, {frame_h}, {count}, {name}_palette, {name}_frames}};")
    return body, cost + 2 * len(palette)


def convert_tiles(path):
    im = Image.open(path).convert("RGBA")
    rows = []
    for name, x0, y0 in TILES:
        values = []
        for y in range(y0, y0 + TILE):
            for x in range(x0, x0 + TILE):
                r, g, b, a = im.getpixel((x, y))
                values.append(0 if a < ALPHA_CUTOFF else swap(rgb565(r, g, b) or NEAR_BLACK))
        rows.append(f"    // {name}\n    {{{words([f'0x{v:04X}' for v in values], 16)}}}")
    body = [f"const uint16_t PROGMEM tile_art[TILE_ART_COUNT][{TILE * TILE}] = {{\n" + ",\n".join(rows) + "};"]
    return body, len(TILES) * TILE * TILE * 2


def convert_splash(name, path):
    im = Image.open(path).convert("RGBA")
    alpha = im.getchannel("A").point(lambda a: 255 if a >= ALPHA_CUTOFF else 0)
    x0, y0, x1, y1 = alpha.getbbox()
    w = x1 - x0
    palette = []
    lookup = {}
    runs = []
    for y in range(y0, y1):
        row = []
        for x in range(x0, x1):
            r, g, b, a = im.getpixel((x, y))
            if a < ALPHA_CUTOFF:
                row.append(0)
                continue
            c = rgb565(r, g, b)
            if c not in lookup:
                palette.append(c)
                lookup[c] = len(palette)
            row.append(lookup[c])
        i = 0
        while i < w:
            j = i
            while j < w and row[j] == row[i] and j - i < 255:
                j += 1
            runs += [j - i, row[i]]
            i = j
    body = [
        f"static const uint16_t PROGMEM {name}_palette[] = {{{', '.join(f'0x{swap(c):04X}' for c in palette)}}};",
        f"static const uint8_t PROGMEM {name}_runs[] = {{{', '.join(str(v) for v in runs)}}};",
        f"const SpriteDef {name} = {{{x0}, {y0}, {w}, {y1 - y0}, {name}_palette, {name}_runs}};",
    ]
    return body, len(runs) + 2 * len(palette)


def main():
    header = [
        "// Generated by tools/convert_sprites.py from src/assets/png/sprites/. Do not edit.",
        "#ifndef SPRITES_H",
        "#define SPRITES_H",
        "",
        "#include <Arduino.h>",
        "",
        "// Splash art: the opaque box inside its canvas, a palette, and each row of the box as",
        "// (length, index) byte pairs. Index 0 is transparent; index i is palette[i - 1].",
        "struct SpriteDef",
        "{",
        "    uint8_t x;",
        "    uint8_t y;",
        "    uint8_t w;",
        "    uint8_t h;",
        "    const uint16_t *palette; // Byte-swapped RGB565",
        "    const uint8_t *runs;",
        "};",
        "",
        "// One frame of a sheet: its opaque box inside the frame, one palette index per pixel, row",
        "// by row. Index 0 is transparent; index i is the sheet's palette[i - 1].",
        "struct FrameDef",
        "{",
        "    uint8_t x;",
        "    uint8_t y;",
        "    uint8_t w;",
        "    uint8_t h;",
        "    const uint8_t *pixels;",
        "};",
        "",
        "struct SheetDef",
        "{",
        "    uint8_t frameW;",
        "    uint8_t frameH;",
        "    uint8_t count;",
        "    const uint16_t *palette; // Byte-swapped RGB565",
        "    const FrameDef *frames;",
        "};",
        "",
        f"static const uint8_t TILE_SIZE = {TILE};",
        "",
        "enum TileArt : uint8_t",
        "{",
    ] + [f"    TILE_ART_{name}," for name, _, _ in TILES] + [
        "    TILE_ART_COUNT",
        "};",
        "// Byte-swapped RGB565, 0x0000 transparent",
        f"extern const uint16_t tile_art[TILE_ART_COUNT][{TILE * TILE}];",
        "",
    ]
    source = [
        "// Generated by tools/convert_sprites.py from src/assets/png/sprites/. Do not edit.",
        '#include "Sprites.h"',
        "",
    ]

    total = 0
    header.append("// Sheets")
    for stem, (fw, fh) in SHEETS.items():
        path = SRC / "sheets" / f"{stem}.png"
        body, cost = convert_sheet(f"sheet_{stem}", path, fw, fh)
        total += cost
        header.append(f"extern const SheetDef sheet_{stem};")
        source += body + [""]
        print(f"sheets/{stem + '.png':22} {cost:7} bytes")
    unlisted = {p.stem for p in (SRC / "sheets").glob("*.png")} - set(SHEETS)
    if unlisted:
        sys.exit(f"sheets without a frame size in SHEETS: {', '.join(sorted(unlisted))}")
    header.append("")

    body, cost = convert_tiles(SRC / "tiles" / "tileset.png")
    total += cost
    source += body + [""]
    print(f"tiles/tileset.png        {cost:7} bytes")

    header.append("// Splash")
    for path in sorted((SRC / "splash").glob("*.png")):
        body, cost = convert_splash(f"splash_{path.stem}", path)
        total += cost
        header.append(f"extern const SpriteDef splash_{path.stem};")
        source += body + [""]
        print(f"splash/{path.name:22} {cost:7} bytes")
    header.append("")

    header.append("#endif // SPRITES_H")
    OUT_H.write_text("\n".join(header) + "\n")
    OUT_CPP.write_text("\n".join(source))
    print(f"total {total} bytes")


if __name__ == "__main__":
    main()
