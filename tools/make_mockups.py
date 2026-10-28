#!/usr/bin/env python3
"""Draws docs/img/<id>.png, a 240x320 picture of each game's screen for the site, from the
games' own sprites and colours instead of a photo of the board.

The pet's tiles are redrawn here after its TileArt.cpp, with the same art colours; the idle
game's combat screen uses its enemy and background PNGs. Decaf Inferno's floor is filmed by the
game's own renderer (its tools/sim/film.sh, which needs g++) and turned upright, since that game
holds the board on its side. Run from the repo root:
python3 tools/make_mockups.py
"""
import random
import subprocess
import tempfile
from pathlib import Path

from PIL import Image, ImageDraw, ImageFont

ROOT = Path(__file__).resolve().parent.parent
IDLE = ROOT.parent / "esp32-idle-game" / "src" / "assets" / "png" / "sprites"
DECAF = ROOT.parent / "esp32-decaf"
# Steps of the solver's route through B1: the mantis charging, the player jumping over him
DECAF_STEPS = 900
OUT = ROOT / "docs" / "img"
W, H = 240, 320
FONT = ImageFont.load_default_imagefont()


def rgb(c):
    return ((c >> 11 & 31) * 255 // 31, (c >> 5 & 63) * 255 // 63, (c & 31) * 255 // 31)


# The Cave theme's slots both games start with
CAVE = {
    "HEADER": 0x0841, "BG": 0x1081, "CARD": 0x20E3, "FRAME": 0x49E6, "TEXT": 0xF75B, "TEXT_2": 0xBD52,
    "GOLD": 0xF5A8, "GOOD": 0x8E4D, "BAD": 0xE309, "INFO": 0x5E39, "SELECT": 0xF5A8,
    "ENEMY_NAME": 0xF52F, "HP_BAR": 0x64EA, "HP_TRACK": 0x2163, "ENEMY_HP_BAR": 0x91C6, "ENEMY_HP_TRACK": 0x28C2,
}
T = {k: rgb(v) for k, v in CAVE.items()}


def text(d, xy, s, color, anchor="la"):
    """The bitmap font only draws from its top left, so the anchor is applied here"""
    left, top, right, bottom = FONT.getbbox(s)
    x, y = xy
    x -= {"l": 0, "m": (right - left) // 2, "r": right - left}[anchor[0]]
    y -= {"a": 0, "m": (bottom - top) // 2 + top}[anchor[1]]
    d.text((x, y), s, font=FONT, fill=color)


def big_text(img, xy, s, color, scale=2):
    """The header's size-2 title: the bitmap font drawn small and scaled without smoothing"""
    box = FONT.getbbox(s)
    small = Image.new("RGBA", (box[2] + 1, box[3] + 1))
    ImageDraw.Draw(small).text((0, 0), s, font=FONT, fill=color)
    small = small.resize((small.width * scale, small.height * scale), Image.NEAREST)
    img.paste(small, (xy[0] - small.width // 2, xy[1]), small)


def header(img, title, back, nxt):
    d = ImageDraw.Draw(img)
    d.rectangle((0, 0, W - 1, 27), fill=T["HEADER"])
    d.line((0, 27, W - 1, 27), fill=T["FRAME"])
    big_text(img, (W // 2, 5), title, T["TEXT"])
    text(d, (4, 6), "< " + back, T["TEXT_2"])
    text(d, (W - 4, 6), nxt + " >", T["TEXT_2"], "ra")


def bar(d, x, y, w, h, frac, fill, track, label):
    d.rectangle((x, y, x + w - 1, y + h - 1), fill=track, outline=T["FRAME"])
    d.rectangle((x + 1, y + 1, x + 1 + int((w - 3) * frac), y + h - 2), fill=fill)
    text(d, (x + w // 2, y + h // 2 + 1), label, T["TEXT"], "mm")


def idle():
    img = Image.new("RGB", (W, H), T["BG"])
    header(img, "Combat", "Zones", "Inventory")
    d = ImageDraw.Draw(img)
    d.rectangle((8, 36, W - 9, 220), fill=T["CARD"], outline=T["FRAME"])
    scale = 2
    bg = Image.open(IDLE / "backgrounds" / "stone.png").convert("RGBA")
    bg = bg.resize((bg.width * scale, bg.height * scale), Image.NEAREST)
    enemy = Image.open(IDLE / "enemies" / "goblin.png").convert("RGBA")
    enemy = enemy.resize((enemy.width * scale, enemy.height * scale), Image.NEAREST)
    left, top = (W - bg.width) // 2, 62
    img.paste(bg, (left, top), bg)
    img.paste(enemy, (left, top), enemy)
    text(d, (W // 2, 44), "GOBLIN", T["ENEMY_NAME"], "ma")
    bar(d, 20, 196, W - 40, 12, 0.62, T["ENEMY_HP_BAR"], T["ENEMY_HP_TRACK"], "HP 31/50")

    d.rectangle((8, 228, W - 9, 312), fill=T["CARD"], outline=T["FRAME"])
    text(d, (16, 236), "You", T["TEXT_2"])
    text(d, (W - 16, 236), "12.4K gold", T["GOLD"], "ra")
    bar(d, 16, 250, W - 32, 12, 0.85, T["HP_BAR"], T["HP_TRACK"], "HP 85/100")
    text(d, (16, 272), "ATK 24   DEF 11", T["TEXT"])
    d.ellipse((W - 46, 272, W - 18, 300), fill=rgb(0xE309), outline=T["TEXT"])
    text(d, (W - 32, 287), "HIT", T["TEXT"], "mm")
    text(d, (16, 290), "Kills 214", T["TEXT_2"])
    return img


# TileArt.cpp's terrain, RGB565 in WorldConfig.h
A = {k: rgb(v) for k, v in {
    "WILD": 0x1A42, "WILD_LEAF": 0x2B44, "WILD_THORN": 0x4A22, "GRASS": 0x3D06, "GRASS_DARK": 0x2C04,
    "GRASS_LIGHT": 0x5E48, "SOIL": 0x7A85, "SOIL_DARK": 0x51A3, "SOIL_WET": 0x4163, "SPROUT": 0x5F2A,
    "LEAF": 0x2D45, "MOUND": 0x8B27, "MOUND_DARK": 0x6204, "HOLE": 0x2081, "WOOD": 0x92E5,
    "WOOD_DARK": 0x6203, "FEED": 0xDDA7, "WATER": 0x2B37, "WATER_LIGHT": 0x7DBC, "EYE": 0x0841,
}.items()}
PETALS = [rgb(c) for c in (0xFB56, 0xFFE0, 0xC81F, 0xFFFF)]
CROP_COLORS = [rgb(c) for c in (0xF71C, 0xFBE0, 0xC8D4, 0xFC40)]
SPECIES = [(rgb(0x9CD3), rgb(0xFDB8), "round"), (rgb(0xC4EC), rgb(0xFFDB), "long"),
           (rgb(0x7BEF), rgb(0xFFFF), "stripe"), (rgb(0xFC83), rgb(0xFFFF), "pointy")]


def ellipse(d, cx, cy, rx, ry, color):
    d.ellipse((cx - rx, cy - ry, cx + rx, cy + ry), fill=color)


def tile(kind, rng, extra=None):
    t = Image.new("RGB", (16, 16))
    d = ImageDraw.Draw(t)

    def grass():
        d.rectangle((0, 0, 15, 15), fill=A["GRASS"])
        for b in range(5):
            x, y = rng.randrange(15), rng.randrange(14)
            d.line((x, y, x, y + 1), fill=A["GRASS_DARK"] if b % 2 else A["GRASS_LIGHT"])

    def soil(wet):
        d.rectangle((0, 0, 15, 15), fill=A["SOIL_WET"] if wet else A["SOIL"])
        for y in (3, 7, 11, 15):
            d.line((1, y, 14, y), fill=A["SOIL_DARK"])

    if kind == "wild":
        d.rectangle((0, 0, 15, 15), fill=A["WILD"])
        for _ in range(4):
            ellipse(d, 2 + rng.randrange(12), 2 + rng.randrange(12), 3, 3, A["WILD_LEAF"])
        d.line((rng.randrange(8), 15, 8 + rng.randrange(8), 2), fill=A["WILD_THORN"])
    elif kind == "grass":
        grass()
    elif kind == "soil":
        soil(False)
    elif kind == "crop":
        stage, color = extra
        soil(stage < 3)
        if stage == 1:
            d.line((8, 8, 8, 12), fill=A["SPROUT"])
            d.point([(7, 8), (9, 7)], fill=A["SPROUT"])
        elif stage == 2:
            d.line((8, 5, 8, 12), fill=A["LEAF"])
            ellipse(d, 5, 8, 3, 2, A["SPROUT"])
            ellipse(d, 11, 7, 3, 2, A["SPROUT"])
        else:
            d.line((8, 3, 8, 11), fill=A["LEAF"])
            ellipse(d, 4, 9, 3, 2, A["SPROUT"])
            ellipse(d, 12, 9, 3, 2, A["SPROUT"])
            ellipse(d, 8, 6, 3, 3, color)
            d.point((7, 5), fill=(255, 255, 255))
    elif kind == "burrow":
        grass()
        ellipse(d, 8, 10, 7, 5, A["MOUND"])
        ellipse(d, 9, 12, 5, 3, A["MOUND_DARK"])
        ellipse(d, 8, 11, 3, 2, A["HOLE"])
    elif kind == "trough":
        grass()
        d.rectangle((1, 7, 14, 13), fill=A["WOOD_DARK"])
        d.rectangle((2, 8, 13, 12), fill=A["WOOD"])
        d.rectangle((3, 8, 12, 9), fill=A["FEED"])
    elif kind == "pond":
        d.rectangle((0, 0, 15, 15), fill=A["GRASS"])
        d.rounded_rectangle((0, 1, 15, 14), 5, fill=A["WATER"])
        d.line((3, 5, 7, 5), fill=A["WATER_LIGHT"])
        d.line((8, 10, 12, 10), fill=A["WATER_LIGHT"])
    elif kind == "flowers":
        grass()
        for f in range(4):
            x, y = 3 + (f % 2) * 8 + rng.randrange(3), 3 + (f // 2) * 8 + rng.randrange(3)
            p = rng.choice(PETALS)
            d.point([(x - 1, y), (x + 1, y), (x, y - 1), (x, y + 1)], fill=p)
            d.point((x, y), fill=A["FEED"])
    return t


def critter(t, species, asleep=False):
    body, accent, shape = SPECIES[species]
    d = ImageDraw.Draw(t)
    cx, by = 8, 11
    if shape == "round":
        for ex in (cx - 3, cx + 2):
            ellipse(d, ex, by - 5, 2, 2, body)
            d.point((ex, by - 5), fill=accent)
    elif shape == "long":
        for ex in (cx - 3, cx + 1):
            d.rectangle((ex, by - 9, ex + 1, by - 4), fill=body)
            d.line((ex, by - 8, ex, by - 5), fill=accent)
    elif shape == "pointy":
        d.polygon([(cx - 5, by - 3), (cx - 3, by - 8), (cx - 1, by - 3)], fill=body)
        d.polygon([(cx + 1, by - 3), (cx + 3, by - 8), (cx + 5, by - 3)], fill=body)
    ellipse(d, cx, by, 5, 4, body)
    ellipse(d, cx, by + 2, 3, 2, accent)
    if shape == "stripe":
        d.line((cx - 3, by - 3, cx + 3, by - 3), fill=accent)
    if asleep:
        d.line((cx + 1, by - 1, cx + 2, by - 1), fill=A["EYE"])
        d.line((cx + 4, 1, cx + 6, 1), fill=(255, 255, 255))
        d.point((cx + 5, 2), fill=(255, 255, 255))
        d.line((cx + 4, 3, cx + 6, 3), fill=(255, 255, 255))
    else:
        d.point((cx + 2, by - 1), fill=A["EYE"])


def pet():
    rng = random.Random(7)
    img = Image.new("RGB", (W, H), T["BG"])
    header(img, "Habitat", "Shop", "Colony")
    cols, rows = 15, 16
    layout = {}
    for y in range(rows):
        for x in range(cols):
            inside = 2 <= x <= 12 and 3 <= y <= 13 and not (x in (2, 12) and y in (3, 13))
            layout[(x, y)] = "grass" if inside else "wild"
    layout.update({(7, 8): "pond", (8, 8): "pond", (3, 4): "burrow", (5, 4): "burrow", (10, 4): "burrow",
                   (4, 4): "flowers", (11, 5): "flowers", (3, 5): "flowers", (9, 4): "flowers",
                   (6, 12): "trough", (11, 11): "flowers", (3, 11): "soil", (4, 11): "soil"})
    crops = {}
    for i, (x, y) in enumerate([(5, 7), (6, 7), (9, 7), (10, 7), (5, 9), (6, 9), (9, 9), (10, 9),
                                (6, 10), (9, 10), (7, 10), (8, 6)]):
        layout[(x, y)] = "crop"
        crops[(x, y)] = (3 if i % 3 == 0 else 1 + i % 2, CROP_COLORS[i % len(CROP_COLORS)])
    critters = {(4, 6): (0, False), (8, 11): (1, False), (3, 4): (0, True), (10, 8): (2, False),
                (11, 9): (3, False), (6, 5): (1, False), (7, 12): (0, False)}
    cursor = (8, 10)
    for (x, y), kind in layout.items():
        t = tile(kind, rng, crops.get((x, y)))
        if (x, y) in critters:
            critter(t, *critters[(x, y)])
        if (x, y) == cursor:
            d = ImageDraw.Draw(t)
            d.rectangle((0, 0, 15, 15), outline=T["SELECT"])
            d.rectangle((1, 1, 14, 14), outline=T["SELECT"])
        img.paste(t, (x * 16, 28 + y * 16))
    d = ImageDraw.Draw(img)
    top = 28 + rows * 16
    d.line((0, top, W - 1, top), fill=T["FRAME"])
    text(d, (4, top + 5), "Clover, ripe", T["GOOD"])
    text(d, (W - 4, top + 5), "OK: harvest +4", T["TEXT_2"], "ra")
    text(d, (4, top + 21), "1.2K coins", T["GOLD"])
    text(d, (W // 2, top + 21), "Food 84/160", T["GOOD"], "ma")
    text(d, (W - 4, top + 21), "Pop 7/8", T["TEXT_2"], "ra")
    return img


def decaf():
    with tempfile.TemporaryDirectory() as tmp:
        shot = Path(tmp) / "frame.png"
        subprocess.run([DECAF / "tools" / "sim" / "film.sh", "frame", "0", str(DECAF_STEPS), str(shot)],
                       check=True, stdout=subprocess.DEVNULL, env={"TMPDIR": tmp, "PATH": "/usr/bin:/bin"})
        play = Image.open(shot).convert("RGB")
    img = Image.new("RGB", (H, W), T["HEADER"])
    img.paste(play, (0, 16))
    d = ImageDraw.Draw(img)
    text(d, (4, 4), "B1 LIMBO", T["GOLD"])
    text(d, (H // 2, 4), "0:15.0", T["TEXT"], "ma")
    text(d, (206, 4), "BOSS", T["BAD"])
    for i in range(3):
        d.rectangle((232 + i * 7, 5, 236 + i * 7, 9), fill=T["BAD"])
    text(d, (H - 4, 4), "DEATHS 0", T["TEXT_2"], "ra")
    # Held on its side with the D-pad on the left, the picture's top is the board's left edge
    return img.rotate(90, expand=True)


if __name__ == "__main__":
    OUT.mkdir(parents=True, exist_ok=True)
    for name, draw in (("idle", idle), ("pet", pet), ("decaf", decaf)):
        draw().save(OUT / f"{name}.png", optimize=True)
        print(OUT / f"{name}.png")
