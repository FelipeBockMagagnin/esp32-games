#!/usr/bin/env python3
"""Draws the site's brand images from the art engraved on the case (hardware/case/brand.scad
in esp32-idle-game), so the console on the page carries exactly what the printed one does:

  docs/img/brand/case_wordmark.png   the front's wordmark, one pixel per engraved pixel
  docs/img/brand/case_back.png       the back's character in its two printed colours
  docs/img/brand/favicon-*.png       the character's head, square, so it is not squashed

Run from the repo root: python3 tools/make_brand.py
"""
import re
from pathlib import Path

from PIL import Image

ROOT = Path(__file__).resolve().parent.parent
SCAD = ROOT.parent / "esp32-idle-game" / "hardware" / "case" / "brand.scad"
OUT = ROOT / "docs" / "img" / "brand"

# export_3mf.py's materials: the case, the logo and the face
LOGO = (0x1A, 0x1A, 0x1A, 255)
FACE = (0x8C, 0x8C, 0x8C, 255)
MASK = (0xF2, 0xF2, 0xF2, 255)


def pattern(name):
    text = SCAD.read_text()
    body = re.search(name + r"\s*=\s*\[(.*?)\];", text, re.S).group(1)
    return re.findall(r'"([.#]+)"', body)


def draw(rows, color, img=None):
    img = img or Image.new("RGBA", (len(rows[0]), len(rows)))
    for y, row in enumerate(rows):
        for x, c in enumerate(row):
            if c == "#":
                img.putpixel((x, y), color)
    return img


def main():
    OUT.mkdir(parents=True, exist_ok=True)
    draw(pattern("brand_wordmark"), LOGO).save(OUT / "case_wordmark.png")

    character = draw(pattern("brand_character"), LOGO)
    draw(pattern("brand_character_face"), FACE, character)
    character.save(OUT / "case_back.png")

    # The head and shoulders from the splash frame, with its mask, padded to a square
    splash = Image.open(OUT / "hood_0.png").convert("RGBA")
    head = splash.crop((0, 0, splash.width, 22))
    side = max(head.size) + 2
    square = Image.new("RGBA", (side, side))
    square.paste(head, ((side - head.width) // 2, (side - head.height) // 2), head)
    for size in (32, 48, 96, 192):
        scale = max(1, size // side)
        icon = square.resize((side * scale, side * scale), Image.NEAREST)
        canvas = Image.new("RGBA", (size, size))
        canvas.paste(icon, ((size - icon.width) // 2, (size - icon.height) // 2), icon)
        canvas.save(OUT / f"favicon-{size}.png")
    # Home screens show no transparency: the brand's grey field behind it
    touch = Image.new("RGBA", (180, 180), (0x5D, 0x5D, 0x5D, 255))
    icon = square.resize((side * 6, side * 6), Image.NEAREST)
    touch.paste(icon, ((180 - icon.width) // 2, (180 - icon.height) // 2), icon)
    touch.convert("RGB").save(OUT / "apple-touch-icon.png")


if __name__ == "__main__":
    main()
