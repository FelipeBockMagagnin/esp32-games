#!/usr/bin/env python3
"""Draws the placeholder art the idle game has no match for into art/, and copies the idle art
Grimhold reuses (enemies, two backgrounds turned into wall textures, the item icons).

Everything here is meant to be replaced by hand-drawn PNGs of the same size and name; the
converter (tools/convert_art.py) only reads art/. Seeded, so a rerun draws the same pixels.

Run from the game's folder: python3 tools/make_placeholders.py
"""
import math
import random
import shutil
from pathlib import Path

from PIL import Image, ImageDraw

ROOT = Path(__file__).resolve().parent.parent
ART = ROOT / "art"
IDLE = ROOT.parent / "idle" / "src" / "assets" / "png"

IDLE_ENEMIES = ["small_rat", "slime", "zombie", "goblin", "spider", "frog_wizard", "big_bad_rat"]
# The item icons, by slot: each series has five tiers, a to e
ICON_SERIES = {
    "weapon": ["sword_01", "sword_02", "sword_03"],
    "focus": ["staff_01", "staff_03", "spellbook_01"],
    "armor": ["armor_01"],
    "helm": ["helmet_01", "helmet_02"],
    "boots": ["boots_01"],
    "ring": ["ring_01", "ring_02", "ring_03"],
    "amulet": ["necklace_01", "necklace_02", "necklace_03"],
}
EXTRA_ICONS = {"potion_hp": "potion_02a", "potion_mp": "potion_02c"}


def rgba(hex_, a=255):
    return ((hex_ >> 16) & 255, (hex_ >> 8) & 255, hex_ & 255, a)


def shade(c, f):
    return tuple(max(0, min(255, int(v * f))) for v in c[:3]) + (c[3],)


def save(im, *parts):
    path = ART.joinpath(*parts)
    path.parent.mkdir(parents=True, exist_ok=True)
    im.save(path)


def bricks(rng, base, mortar, rows=8, cols=4, jitter=0.18, cracks=0):
    im = Image.new("RGBA", (64, 64), rgba(mortar))
    px = im.load()
    bh = 64 // rows
    bw = 64 // cols
    for r in range(rows):
        offset = (bw // 2) * (r % 2)
        for c in range(-1, cols + 1):
            x0 = c * bw + offset
            tone = 1 + rng.uniform(-jitter, jitter)
            for y in range(r * bh + 1, (r + 1) * bh):
                for x in range(x0 + 1, x0 + bw):
                    if 0 <= x < 64:
                        f = tone * (1 + rng.uniform(-0.08, 0.08))
                        if y == r * bh + 1:
                            f *= 1.18
                        if y == (r + 1) * bh - 1 or x == x0 + bw - 1:
                            f *= 0.72
                        px[x, y] = shade(rgba(base), f)
    for _ in range(cracks):
        x, y = rng.randrange(64), rng.randrange(64)
        for _ in range(rng.randrange(6, 16)):
            if 0 <= x < 64 and 0 <= y < 64:
                px[x, y] = shade(rgba(mortar), 0.6)
            x += rng.choice((-1, 0, 1))
            y += 1
    return im


def wall_crypt(rng):
    im = bricks(rng, 0x4a4652, 0x1c1a20, rows=8, cols=3, cracks=6)
    d = ImageDraw.Draw(im)
    # Moss along the foot
    for x in range(64):
        for y in range(56 + rng.randrange(0, 4), 64):
            if rng.random() < 0.55:
                d.point((x, y), fill=shade(rgba(0x2f4a2a), rng.uniform(0.7, 1.2)))
    return im


def wall_bone(rng):
    im = Image.new("RGBA", (64, 64), rgba(0x2a2018))
    d = ImageDraw.Draw(im)
    for _ in range(70):
        x, y = rng.randrange(-4, 64), rng.randrange(-4, 64)
        if rng.random() < 0.35:
            # A skull
            d.ellipse((x, y, x + 9, y + 8), fill=shade(rgba(0xd8cfb4), rng.uniform(0.75, 1.05)))
            d.rectangle((x + 2, y + 7, x + 7, y + 10), fill=shade(rgba(0xc8bea0), rng.uniform(0.75, 1.0)))
            d.point((x + 3, y + 4), fill=rgba(0x140c08))
            d.point((x + 6, y + 4), fill=rgba(0x140c08))
            d.rectangle((x + 3, y + 3, x + 3, y + 5), fill=rgba(0x140c08))
            d.rectangle((x + 6, y + 3, x + 6, y + 5), fill=rgba(0x140c08))
        else:
            # A long bone
            angle = rng.uniform(0, math.pi)
            length = rng.randrange(8, 16)
            x1, y1 = x + math.cos(angle) * length, y + math.sin(angle) * length
            tone = shade(rgba(0xcfc4a6), rng.uniform(0.6, 1.0))
            d.line((x, y, x1, y1), fill=tone, width=2)
            d.ellipse((x - 2, y - 2, x + 2, y + 2), fill=tone)
            d.ellipse((x1 - 2, y1 - 2, x1 + 2, y1 + 2), fill=tone)
    return im


def wall_blood(rng):
    im = bricks(rng, 0x5a2626, 0x1a0808, rows=8, cols=4, cracks=4)
    px = im.load()
    for _ in range(9):
        x = rng.randrange(64)
        length = rng.randrange(10, 40)
        width = rng.choice((1, 1, 2))
        for y in range(length):
            for w in range(width):
                if x + w < 64:
                    px[x + w, y] = shade(rgba(0x8a0f12), rng.uniform(0.7, 1.1))
        if x + 1 < 64:
            px[x, length] = rgba(0xa01418)
    return im


def wall_portal(rng, phase):
    im = Image.new("RGBA", (64, 64), rgba(0x15101c))
    px = im.load()
    # A stone arch with a swirling rift inside
    for y in range(64):
        for x in range(64):
            dx, dy = (x - 32) / 22, (y - 36) / 28
            r = math.hypot(dx, dy)
            if y > 8 and r < 1:
                a = math.atan2(dy, dx) + phase * math.pi / 2 + r * 5
                v = 0.5 + 0.5 * math.sin(a * 3)
                core = 1 - r
                px[x, y] = (int(60 + 140 * v * core + 40 * core), int(20 + 40 * v), int(110 + 140 * core), 255)
            elif r < 1.22 and y > 4:
                px[x, y] = shade(rgba(0x5a5466), 0.8 + 0.3 * ((x // 4 + y // 6) % 2))
            else:
                px[x, y] = shade(rgba(0x2a2632), rng.uniform(0.8, 1.1))
    return im


def wall_sealed(rng):
    im = wall_crypt(rng)
    d = ImageDraw.Draw(im)
    d.rectangle((14, 10, 49, 63), fill=rgba(0x2a2230))
    for y in range(12, 64, 8):
        d.line((14, y, 49, y), fill=rgba(0x5c4a2a))
    d.line((14, 10, 49, 63), fill=rgba(0x7a5a2a), width=3)
    d.line((49, 10, 14, 63), fill=rgba(0x7a5a2a), width=3)
    d.ellipse((26, 30, 37, 41), fill=rgba(0xb02020))
    return im


def skeleton(rng):
    im = Image.new("RGBA", (64, 64), (0, 0, 0, 0))
    d = ImageDraw.Draw(im)
    bone = rgba(0xe2dac4)
    dark = rgba(0x8a826e)
    d.ellipse((24, 4, 39, 19), fill=bone)
    d.rectangle((27, 16, 36, 22), fill=bone)
    d.ellipse((27, 9, 30, 13), fill=rgba(0x200808))
    d.ellipse((33, 9, 36, 13), fill=rgba(0x200808))
    d.point((28, 11), fill=rgba(0xff3020))
    d.point((34, 11), fill=rgba(0xff3020))
    for x in range(28, 36, 2):
        d.line((x, 19, x, 22), fill=dark)
    d.line((31, 22, 31, 42), fill=bone, width=3)
    for i, y in enumerate(range(25, 39, 4)):
        w = 9 - i
        d.arc((31 - w, y - 3, 31 + w, y + 3), 0, 180, fill=bone, width=2)
    d.ellipse((24, 40, 39, 46), fill=bone)
    d.line((25, 24, 18, 36), fill=bone, width=2)
    d.line((18, 36, 16, 46), fill=bone, width=2)
    d.line((38, 24, 45, 34), fill=bone, width=2)
    d.line((45, 34, 50, 28), fill=bone, width=2)
    # A rusty sword in the right hand
    d.line((50, 30, 58, 6), fill=rgba(0x9a8a78), width=2)
    d.line((47, 31, 53, 29), fill=rgba(0x6a4a2a), width=2)
    d.line((27, 46, 25, 62), fill=bone, width=2)
    d.line((36, 46, 38, 62), fill=bone, width=2)
    d.rectangle((22, 61, 27, 63), fill=dark)
    d.rectangle((36, 61, 41, 63), fill=dark)
    # A rag of a loincloth
    d.polygon(((25, 44), (38, 44), (35, 52), (28, 50)), fill=rgba(0x4a3a5a))
    return im


def wraith(rng):
    im = Image.new("RGBA", (64, 64), (0, 0, 0, 0))
    px = im.load()
    for y in range(64):
        for x in range(64):
            # A hooded figure that thins into smoke at the bottom
            cx = 32 + math.sin(y / 6) * 2
            half = 10 + y * 0.28 if y < 46 else 23 - (y - 46) * 0.9
            if abs(x - cx) < half and y > 4:
                edge = (half - abs(x - cx)) / half
                fade = 1 if y < 46 else max(0, 1 - (y - 46) / 18)
                if rng.random() < fade:
                    v = 0.4 + 0.6 * edge
                    px[x, y] = (int(40 * v), int(50 * v), int(80 + 60 * v), 255)
    d = ImageDraw.Draw(im)
    d.ellipse((24, 10, 40, 30), fill=rgba(0x05040a))
    d.ellipse((27, 17, 30, 20), fill=rgba(0x9af0ff))
    d.ellipse((34, 17, 37, 20), fill=rgba(0x9af0ff))
    d.line((14, 30, 6, 40), fill=rgba(0x3a4a7a), width=3)
    d.line((50, 30, 58, 40), fill=rgba(0x3a4a7a), width=3)
    for hand in ((4, 40), (56, 40)):
        d.ellipse((hand[0], hand[1], hand[0] + 5, hand[1] + 5), fill=rgba(0x9aa8c0))
    return im


def lich(rng):
    im = Image.new("RGBA", (64, 64), (0, 0, 0, 0))
    d = ImageDraw.Draw(im)
    robe = rgba(0x3a1a4a)
    d.polygon(((32, 18), (12, 63), (52, 63)), fill=robe)
    d.polygon(((32, 22), (22, 63), (42, 63)), fill=shade(robe, 1.4))
    d.ellipse((24, 8, 39, 23), fill=rgba(0xd8d0bc))
    d.ellipse((27, 13, 30, 17), fill=rgba(0x10ff60))
    d.ellipse((33, 13, 36, 17), fill=rgba(0x10ff60))
    d.rectangle((28, 20, 35, 23), fill=rgba(0xb8b09c))
    # Crown
    d.polygon(((23, 10), (25, 2), (28, 8), (32, 0), (36, 8), (39, 2), (41, 10)), fill=rgba(0xd8a828))
    d.point((32, 4), fill=rgba(0xff2040))
    # Staff with a skull orb
    d.line((52, 63, 54, 10), fill=rgba(0x5a3a1a), width=2)
    d.ellipse((49, 3, 59, 13), fill=rgba(0x40ff90))
    d.ellipse((51, 5, 57, 11), fill=rgba(0xd0ffe0))
    d.line((40, 30, 53, 26), fill=shade(robe, 1.2), width=4)
    d.line((24, 30, 14, 40), fill=shade(robe, 1.2), width=4)
    d.ellipse((10, 38, 16, 44), fill=rgba(0xd8d0bc))
    return im


def orb(size, inner, outer):
    im = Image.new("RGBA", (size, size), (0, 0, 0, 0))
    px = im.load()
    c = (size - 1) / 2
    for y in range(size):
        for x in range(size):
            r = math.hypot(x - c, y - c) / (size / 2)
            if r < 1:
                t = r * r
                col = tuple(int(inner[i] * (1 - t) + outer[i] * t) for i in range(3))
                px[x, y] = col + (255,)
    return im


def spark(rng):
    im = Image.new("RGBA", (24, 24), (0, 0, 0, 0))
    d = ImageDraw.Draw(im)
    for _ in range(10):
        a = rng.uniform(0, math.tau)
        r = rng.uniform(6, 11)
        d.line((12, 12, 12 + math.cos(a) * r, 12 + math.sin(a) * r), fill=rgba(0xffd070), width=1)
    d.ellipse((8, 8, 15, 15), fill=rgba(0xfff4c0))
    return im


def sword_frame(swing):
    # 72x60, drawn at 2x over the bottom right of the view
    im = Image.new("RGBA", (72, 60), (0, 0, 0, 0))
    d = ImageDraw.Draw(im)
    blade = rgba(0xc8ccd8)
    edge = rgba(0xf4f6ff)
    dark = rgba(0x6a6e80)
    if not swing:
        tip, hilt = (30, 2), (58, 46)
    else:
        tip, hilt = (2, 30), (44, 44)
        for i in range(4):
            d.arc((4 - i * 3, 6 + i * 2, 70, 80), 200, 290, fill=rgba(0x90a0c0, 255), width=1)
    d.line((hilt, tip), fill=dark, width=7)
    d.line((hilt, tip), fill=blade, width=5)
    d.line(((hilt[0] - 1, hilt[1] - 2), (tip[0] - 1, tip[1])), fill=edge, width=1)
    gx, gy = hilt
    ang = math.atan2(tip[1] - gy, tip[0] - gx) + math.pi / 2
    gx2, gy2 = math.cos(ang) * 10, math.sin(ang) * 10
    d.line((gx - gx2, gy - gy2, gx + gx2, gy + gy2), fill=rgba(0x8a6a2a), width=4)
    d.line((gx, gy, gx + 10, gy + 14), fill=rgba(0x3a2a1a), width=6)
    d.ellipse((gx + 6, gy + 10, gx + 18, gy + 22), fill=rgba(0x9a7a5a))
    d.ellipse((gx + 8, gy + 12, gx + 16, gy + 20), fill=rgba(0x5a3a2a))
    return im


def hand_frame(cast):
    # 56x52, drawn at 2x over the bottom left of the view: a bare hand out of an iron bracer, palm
    # up and cupped while charging, fingers spread when the spell leaves
    im = Image.new("RGBA", (56, 52), (0, 0, 0, 0))
    d = ImageDraw.Draw(im)
    skin, skin_dark, skin_light = rgba(0xb08a70), rgba(0x6a4a3a), rgba(0xd0a888)
    iron, iron_dark = rgba(0x5a5e6a), rgba(0x2a2c34)
    d.polygon(((8, 52), (16, 34), (38, 34), (46, 52)), fill=iron_dark)
    d.polygon(((11, 52), (18, 36), (36, 36), (43, 52)), fill=iron)
    for y in (40, 46):
        d.line((14, y, 40, y), fill=iron_dark)
    d.ellipse((14, 20, 40, 40), fill=skin_dark)
    d.ellipse((15, 21, 39, 38), fill=skin)
    d.ellipse((20, 25, 32, 33), fill=skin_light)
    spread = 7 if cast else 4
    reach = 15 if cast else 9
    for i in range(4):
        fx = 17 + i * spread - (3 * spread - 18) // 2
        top = 22 - reach + abs(i - 1.5) * 2
        d.rounded_rectangle((fx - 2, top, fx + 3, 28), 2, fill=skin_dark)
        d.rounded_rectangle((fx - 1, top + 1, fx + 2, 27), 1, fill=skin)
        d.point((fx, top + 2), fill=skin_light)
    d.line((15, 30, 7 if cast else 10, 22 if cast else 24), fill=skin_dark, width=5)
    d.line((15, 30, 7 if cast else 10, 22 if cast else 24), fill=skin, width=3)
    return im


def copy_idle():
    for name in IDLE_ENEMIES:
        shutil.copy(IDLE / "sprites" / "enemies" / f"{name}.png", ART / "enemies" / f"{name}.png")
    stone = Image.open(IDLE / "sprites" / "backgrounds" / "stone.png").convert("RGBA")
    sewers = Image.open(IDLE / "sprites" / "backgrounds" / "sewers.png").convert("RGBA")
    # The backgrounds are a wall over a floor: the wall's band, tiled to a square texture
    tile = stone.crop((0, 0, 64, 32))
    im = Image.new("RGBA", (64, 64))
    im.paste(tile, (0, 0))
    im.paste(tile.transpose(Image.FLIP_LEFT_RIGHT), (0, 32))
    save(im, "walls", "stone.png")
    tile = sewers.crop((24, 0, 56, 32))
    im = Image.new("RGBA", (64, 64))
    for ox in (0, 32):
        for oy in (0, 32):
            im.paste(tile, (ox, oy))
    save(im, "walls", "sewer.png")
    for slot, series in ICON_SERIES.items():
        for s in series:
            for tier in "abcde":
                src = IDLE / "icons" / f"{s}{tier}.png"
                if not src.exists():
                    # The idle art misnames one file (staff_02ab); fall back to the series' first
                    src = IDLE / "icons" / f"{s}a.png"
                shutil.copy(src, ART / "items" / f"{slot}_{s}{tier}.png")
    for name, src in EXTRA_ICONS.items():
        shutil.copy(IDLE / "icons" / f"{src}.png", ART / "items" / f"{name}.png")


def main():
    for sub in ("walls", "enemies", "fx", "hand", "items"):
        (ART / sub).mkdir(parents=True, exist_ok=True)
    rng = random.Random(1347)
    save(wall_crypt(rng), "walls", "crypt.png")
    save(wall_bone(rng), "walls", "bone.png")
    save(wall_blood(rng), "walls", "blood.png")
    save(wall_portal(rng, 0), "walls", "portal_0.png")
    save(wall_portal(rng, 1), "walls", "portal_1.png")
    save(wall_sealed(rng), "walls", "sealed.png")
    save(skeleton(rng), "enemies", "skeleton.png")
    save(wraith(rng), "enemies", "wraith.png")
    save(lich(rng), "enemies", "lich.png")
    save(orb(20, (255, 250, 200), (255, 90, 10)), "fx", "fireball.png")
    save(orb(14, (220, 255, 220), (60, 200, 60)), "fx", "bolt.png")
    save(spark(rng), "fx", "spark.png")
    save(sword_frame(False), "hand", "sword_idle.png")
    save(sword_frame(True), "hand", "sword_swing.png")
    save(hand_frame(False), "hand", "hand_charge.png")
    save(hand_frame(True), "hand", "hand_cast.png")
    copy_idle()


if __name__ == "__main__":
    main()
