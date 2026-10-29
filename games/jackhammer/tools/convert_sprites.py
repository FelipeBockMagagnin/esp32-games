#!/usr/bin/env python3
"""Converts the Unity game's PNGs in src/assets/png/sprites/ into src/assets/Sprites.{h,cpp}.

Each sprite is scaled to the size it had in the Unity scene: its pixels at 100 per unit, times
its prefab's scale, at PX_PER_UNIT (src/jack/Config.h) pixels per unit. Frames are cropped to
the opaque box they share, quantized to one palette per art, and stored as palette indices
(0 transparent). The pivot is the Unity sprite's, so the player turns around the same point.

The backdrop is the centre square of the Creepy background at half the arena's size, drawn 2x.
Each style also gets a 16x16 RGB565 icon of its enemy for the shop.

Particle textures are RGB565, at most 16 px a side, scaled to each particle's size at runtime.
Glows are white with an alpha per pixel: sprites the Unity Creepy character carried as children
(a fog and a beam of light), placed, scaled and turned the way the prefab had them, with the
character's pivot recorded so they turn with it.

Run from the repo root: python3 tools/convert_sprites.py
"""
from pathlib import Path

from PIL import Image

ROOT = Path(__file__).resolve().parent.parent
SRC = ROOT / "src" / "assets" / "png" / "sprites"
OUT_H = ROOT / "src" / "assets" / "Sprites.h"
OUT_CPP = ROOT / "src" / "assets" / "Sprites.cpp"

PX_PER_UNIT = 20
ALPHA_CUTOFF = 110
MAX_COLORS = 64
ICON = 16
BACKDROP = 120
NEAR_BLACK = 0x0020

# name, file, frame rects (x, top, w, h) or None for the whole image, pivot (x, y from the
# bottom, as fractions of the frame, from the .png.meta), prefab scale, pixel art
ARTS = [
    ("player_pixel", "player_pixel.png", None, (0.5032322, 0.2831111), 1.0, True),
    ("player_pen", "player_pen.png", [(0, 0, 398, 770), (397, 0, 398, 770), (794, 0, 398, 770)], (0.5, 0.25), 0.27, False),
    ("player_watercolor", "player_watercolor.png", [(0, 0, 402, 770), (402, 0, 402, 770), (804, 0, 402, 770)], (0.5, 0.25), 0.27, False),
    ("player_creepy", "player_creepy.png", None, (0.5, 0.23), 0.52, False),
    ("player_neon", "player_neon.png", [(77, 850 - 195 - 451, 269, 451)], (0.5, 0.25), 0.5, False),
    ("player_easter", "player_easter.png", None, (0.52477705, 0.2323848), 0.25, False),
    ("enemy_pixel", "enemy_pixel.png", None, (0.5, 0.5), 1.0, True),
    ("enemy_pen", "enemy_pen.png", [(0, 0, 498, 500), (498, 0, 498, 500), (996, 0, 498, 500)], (0.5, 0.5), 0.25, False),
    ("enemy_watercolor", "enemy_watercolor.png", [(0, 0, 491, 500), (491, 0, 491, 500), (982, 0, 491, 500)], (0.5, 0.5), 0.25, False),
    ("enemy_creepy", "enemy_creepy.png", None, (0.5, 0.5), 0.52, False),
    ("enemy_neon_pink", "enemy_neon_pink.png", None, (0.5, 0.5), 0.5, False),
    ("enemy_neon_cyan", "enemy_neon_cyan.png", None, (0.5, 0.5), 0.5, False),
    ("enemy_neon_orange", "enemy_neon_orange.png", None, (0.5, 0.5), 0.5, False),
    ("enemy_easter", "enemy_easter.png", None, (0.5, 0.5), 0.2, False),
]
# name, file or None for a plain square
PARTICLES = [
    ("square", None),
    ("circle", "particle_circle.png"),
    ("pen", "particle_pen.png"),
    ("watercolor", "particle_watercolor.png"),
    ("egg", "particle_egg.png"),
]
PARTICLE_SIDE = 16

# name, file, the child's local position and scale, its rotation in degrees (z, counterclockwise),
# its colour's alpha, and the parent's scale (MainCharCreepy.prefab)
GLOWS = [
    ("glow_creepy_fog", "glow_creepy_fog.png", (0.03, 0.7), (2.280282, 3.238949), 0.0, 0.43529412, 0.52),
    ("glow_creepy_light", "glow_creepy_light.png", (1.57, 11.86), (8.638078, 12.139137), -175.6, 0.3647059, 0.52),
]
GLOW_CUTOFF = 6

ICONS = ["enemy_pixel", "enemy_pen", "enemy_watercolor", "enemy_creepy", "enemy_neon_pink", "enemy_easter"]


def rgb565(r, g, b):
    return ((r >> 3) << 11) | ((g >> 2) << 5) | (b >> 3)


def swap(c):
    return ((c >> 8) | (c << 8)) & 0xFFFF


def scaled(im, size, pixel_art):
    if pixel_art:
        return im.resize(size, Image.NEAREST)
    # Premultiplied, so transparent pixels do not bleed their colour into the edges
    return im.convert("RGBa").resize(size, Image.BOX).convert("RGBA")


def load_frames(file, rects, pivot, scale, pixel_art):
    sheet = Image.open(SRC / file).convert("RGBA")
    rects = rects or [(0, 0, sheet.width, sheet.height)]
    factor = scale * PX_PER_UNIT / 100
    frames = []
    for x, top, w, h in rects:
        frame = sheet.crop((x, top, x + w, top + h))
        size = (max(1, round(w * factor)), max(1, round(h * factor)))
        frames.append(scaled(frame, size, pixel_art))
    w, h = frames[0].size
    px, py = pivot[0] * w, h - pivot[1] * h
    # The shared opaque box, so frames stay aligned with each other and with the pivot
    box = None
    for f in frames:
        b = f.getchannel("A").point(lambda a: 255 if a >= ALPHA_CUTOFF else 0).getbbox()
        if b:
            box = b if box is None else (min(box[0], b[0]), min(box[1], b[1]), max(box[2], b[2]), max(box[3], b[3]))
    frames = [f.crop(box) for f in frames]
    return frames, round(px - box[0]), round(py - box[1])


def quantize(frames):
    w, h = frames[0].size
    strip = Image.new("RGB", (w, h * len(frames)))
    mask = []
    for i, f in enumerate(frames):
        strip.paste(f.convert("RGB"), (0, i * h))
        mask += [f.getpixel((x, y))[3] >= ALPHA_CUTOFF for y in range(h) for x in range(w)]
    opaque = [strip.getpixel((i % w, i // w)) for i, m in enumerate(mask) if m]
    if len(set(opaque)) <= MAX_COLORS:
        palette = sorted(set(opaque))
        lookup = {c: i for i, c in enumerate(palette)}
        indices = [lookup[strip.getpixel((i % w, i // w))] + 1 if m else 0 for i, m in enumerate(mask)]
    else:
        q = strip.quantize(MAX_COLORS, method=Image.Quantize.MEDIANCUT, dither=Image.Dither.NONE)
        raw = q.getpalette()[: MAX_COLORS * 3]
        palette = [tuple(raw[i * 3 : i * 3 + 3]) for i in range(MAX_COLORS)]
        indices = [q.getpixel((i % w, i // w)) + 1 if m else 0 for i, m in enumerate(mask)]
    # The two colours used most, for the particles of a kill
    counts = {}
    for i in indices:
        if i:
            counts[i] = counts.get(i, 0) + 1
    common = sorted(counts, key=counts.get, reverse=True)[:2] or [1]
    accents = [palette[i - 1] for i in common]
    if len(accents) == 1:
        accents.append(accents[0])
    return palette, indices, accents


def icon_words(frame):
    alpha = frame.getchannel("A").point(lambda a: 255 if a >= ALPHA_CUTOFF else 0)
    im = frame.crop(alpha.getbbox())
    s = ICON / max(im.size)
    small = scaled(im, (max(1, round(im.width * s)), max(1, round(im.height * s))), False)
    icon = Image.new("RGBA", (ICON, ICON))
    icon.paste(small, ((ICON - small.width) // 2, (ICON - small.height) // 2))
    out = []
    for y in range(ICON):
        for x in range(ICON):
            r, g, b, a = icon.getpixel((x, y))
            out.append(0 if a < ALPHA_CUTOFF else swap(rgb565(r, g, b) or NEAR_BLACK))
    return out


def particle_words(file):
    if file is None:
        return PARTICLE_SIDE, PARTICLE_SIDE, [swap(0xFFFF)] * (PARTICLE_SIDE * PARTICLE_SIDE)
    im = Image.open(SRC / file).convert("RGBA")
    im = im.crop(im.getchannel("A").point(lambda a: 255 if a >= ALPHA_CUTOFF else 0).getbbox())
    s = PARTICLE_SIDE / max(im.size)
    im = scaled(im, (max(1, round(im.width * s)), max(1, round(im.height * s))), False)
    out = []
    for y in range(im.height):
        for x in range(im.width):
            r, g, b, a = im.getpixel((x, y))
            out.append(0 if a < ALPHA_CUTOFF else swap(rgb565(r, g, b) or NEAR_BLACK))
    return im.width, im.height, out


def glow(file, position, scale, angle, alpha, parent):
    im = Image.open(SRC / file).convert("RGBA")
    factor = parent * PX_PER_UNIT / 100
    im = scaled(im, (round(im.width * scale[0] * factor), round(im.height * scale[1] * factor)), False)
    im = im.rotate(angle, resample=Image.BICUBIC, expand=True)
    # Its centre, from the character's pivot, in screen pixels (y down)
    cx = position[0] * parent * PX_PER_UNIT
    cy = -position[1] * parent * PX_PER_UNIT
    a = im.getchannel("A").point(lambda v: round(v * alpha))
    box = a.point(lambda v: 255 if v >= GLOW_CUTOFF else 0).getbbox()
    pivot_x = im.width / 2 - cx - box[0]
    pivot_y = im.height / 2 - cy - box[1]
    a = a.crop(box)
    return a.width, a.height, round(pivot_x), round(pivot_y), list(a.tobytes())


def backdrop():
    im = Image.open(SRC / "backdrop_creepy.png").convert("RGB")
    side = min(im.size)
    left, top = (im.width - side) // 2, (im.height - side) // 2
    im = im.crop((left, top, left + side, top + side)).resize((BACKDROP, BACKDROP), Image.BOX)
    q = im.quantize(MAX_COLORS, method=Image.Quantize.MEDIANCUT, dither=Image.Dither.NONE)
    raw = q.getpalette()[: MAX_COLORS * 3]
    palette = [tuple(raw[i * 3 : i * 3 + 3]) for i in range(MAX_COLORS)]
    return palette, list(q.tobytes())


def array(values, fmt, per_line=24):
    lines = []
    for i in range(0, len(values), per_line):
        lines.append("    " + ", ".join(fmt(v) for v in values[i : i + per_line]) + ",")
    return "\n".join(lines)


def main():
    header = [
        "// Generated by tools/convert_sprites.py; do not edit",
        "#ifndef SPRITES_H",
        "#define SPRITES_H",
        "",
        '#include "Art.h"',
        "",
    ]
    body = ["// Generated by tools/convert_sprites.py; do not edit", '#include "Sprites.h"', ""]
    total = 0
    firsts = {}
    for name, file, rects, pivot, scale, pixel_art in ARTS:
        frames, px, py = load_frames(file, rects, pivot, scale, pixel_art)
        firsts[name] = frames[0]
        palette, indices, accents = quantize(frames)
        w, h = frames[0].size
        ident = "ART_" + name.upper()
        header.append(f"extern const Art {ident}; // {w}x{h}, {len(frames)} frame(s)")
        body.append(f"static const uint16_t PROGMEM {name}_palette[] = {{")
        body.append(array([swap(rgb565(*c)) for c in palette], lambda v: f"0x{v:04X}", 12))
        body.append("};")
        body.append(f"static const uint8_t PROGMEM {name}_pixels[] = {{")
        body.append(array(indices, str, 32))
        body.append("};")
        acc = ", ".join(f"0x{swap(rgb565(*c)):04X}" for c in accents)
        body.append(
            f"const Art {ident} = {{{w}, {h}, {px}, {py}, {len(frames)}, {name}_palette, {name}_pixels, {{{acc}}}}};"
        )
        body.append("")
        total += len(indices) + len(palette) * 2

    palette, indices = backdrop()
    header.append(f"extern const Backdrop BACKDROP_CREEPY; // {BACKDROP}x{BACKDROP}, drawn 2x")
    body.append("static const uint16_t PROGMEM backdrop_creepy_palette[] = {")
    body.append(array([swap(rgb565(*c)) for c in palette], lambda v: f"0x{v:04X}", 12))
    body.append("};")
    body.append("static const uint8_t PROGMEM backdrop_creepy_pixels[] = {")
    body.append(array(indices, str, 32))
    body.append("};")
    body.append(f"const Backdrop BACKDROP_CREEPY = {{{BACKDROP}, backdrop_creepy_palette, backdrop_creepy_pixels}};")
    body.append("")
    total += len(indices) + len(palette) * 2

    for name, file in PARTICLES:
        w, h, words = particle_words(file)
        ident = f"PARTICLE_{name.upper()}"
        header.append(f"extern const ParticleTexture {ident}; // {w}x{h}")
        body.append(f"static const uint16_t PROGMEM particle_{name}_pixels[] = {{")
        body.append(array(words, lambda v: f"0x{v:04X}", 16))
        body.append("};")
        body.append(f"const ParticleTexture {ident} = {{{w}, {h}, particle_{name}_pixels}};")
        total += len(words) * 2

    for name, file, position, scale, angle, alpha, parent in GLOWS:
        w, h, px, py, alphas = glow(file, position, scale, angle, alpha, parent)
        ident = name.upper()
        header.append(f"extern const Glow {ident}; // {w}x{h}")
        body.append(f"static const uint8_t PROGMEM {name}_alpha[] = {{")
        body.append(array(alphas, str, 32))
        body.append("};")
        body.append(f"const Glow {ident} = {{{w}, {h}, {px}, {py}, {name}_alpha}};")
        total += len(alphas)

    for name in ICONS:
        ident = f"ICON_{name.upper()}"
        header.append(f"extern const uint16_t {ident}[]; // {ICON}x{ICON} RGB565, byte-swapped")
        body.append(f"const uint16_t PROGMEM {ident}[] = {{")
        body.append(array(icon_words(firsts[name]), lambda v: f"0x{v:04X}", 16))
        body.append("};")
        total += ICON * ICON * 2

    header += ["", "#endif // SPRITES_H", ""]
    body.append("")
    OUT_H.write_text("\n".join(header))
    OUT_CPP.write_text("\n".join(body))
    print(f"{len(ARTS)} arts, a backdrop, {len(PARTICLES)} particles, {len(GLOWS)} glows and {len(ICONS)} icons, "
          f"{total} bytes")


if __name__ == "__main__":
    main()
