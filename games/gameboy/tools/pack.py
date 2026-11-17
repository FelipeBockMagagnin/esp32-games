# PlatformIO pre-script: packs the ROMs of this env's tier (custom_tier: lite or max) from
# library.json and roms/ into one blob, linked into flash as gb_pack_start..gb_pack_end.
# src/emu/Pack.h reads it; the two must change together.
#
# Layout, little-endian: "GBPK", u16 count, u16 version, then `count` 64-byte entries
# (u32 offset, u32 size, u32 id, u8 ok button, 3 reserved, char title[24], char author[24]),
# then each ROM at a 4-byte boundary. The id is FNV-1a of the slug, and names the game's save.
import json
import os
import struct
import unicodedata
from os.path import join, realpath

Import("env")

VERSION = 1
ENTRY = 64
TEXT = 24

# The max edition builds from games/gameboymax but packs this folder's ROMs
game = realpath(env.subst("$PROJECT_DIR/../gameboy"))
tier = env.GetProjectOption("custom_tier")
if tier not in ("lite", "max"):
    raise SystemExit(f"custom_tier must be lite or max, not {tier!r}")


def fnv1a(text):
    h = 2166136261
    for b in text.encode():
        h = ((h ^ b) * 16777619) & 0xFFFFFFFF
    return h


# The board's only font has no accents
def ascii_field(text):
    plain = unicodedata.normalize("NFKD", text).encode("ascii", "ignore")
    return plain[: TEXT - 1].ljust(TEXT, b"\0")


library = json.load(open(join(game, "library.json")))
games = [g for g in library if tier == "max" or g["lite"]]

head = bytearray(b"GBPK" + struct.pack("<HH", len(games), VERSION))
body = bytearray()
start = len(head) + ENTRY * len(games)
for g in games:
    rom = open(join(game, "roms", g["slug"] + ".gb"), "rb").read()
    body += b"\0" * (-(start + len(body)) % 4)
    head += struct.pack("<III", start + len(body), len(rom), fnv1a(g["slug"]))
    head += bytes([1 if g.get("ok") == "B" else 0, 0, 0, 0])
    # The board's list fits 16 characters of a name: `name` and `by` shorten the long ones
    head += ascii_field(g.get("name", g["title"])) + ascii_field(g.get("by", g["author"]))
    body += rom
blob = bytes(head + body)

build = env.subst("$BUILD_DIR")
os.makedirs(build, exist_ok=True)
pack_path = join(build, "gbpack.bin")
asm_path = join(build, "gbpack.S")
try:
    unchanged = open(pack_path, "rb").read() == blob
except OSError:
    unchanged = False
if not unchanged:
    open(pack_path, "wb").write(blob)
open(asm_path, "w").write(
    '.section .rodata.gbpack,"a"\n.balign 4\n.global gb_pack_start\ngb_pack_start:\n'
    f'.incbin "{pack_path}"\n.global gb_pack_end\ngb_pack_end:\n'
)
obj = env.Object(join(build, "gbpack.o"), asm_path)
env.Depends(obj, pack_path)
env.Append(PIOBUILDFILES=obj)
print(f"Game Boy pack ({tier}): {len(games)} games, {len(blob) // 1024} KB")
