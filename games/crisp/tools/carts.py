# Writes CartTable.inc, the table of games src/play/Carts.c includes, from library.json in its
# order, and builds each listed carts/<slug>.c beside Carts.c, with src/'s flags. A file in carts/
# that library.json does not name is an error, so nothing is built that the list does not show.
#
# As a PlatformIO pre-script it writes into the build folder (tools/overlay.py hands overlay.ld to
# the linker); run by hand it is the PC harness's:
#   python3 tools/carts.py <out dir> [library.json] [carts dir]
import json
import os
import re
import sys
from os.path import basename, dirname, join

CONTROLS = {"tap": "CONTROLS_TAP", "arrows": "CONTROLS_ARROWS", "slide": "CONTROLS_SLIDE"}
KEYS = {"up": "CART_KEY_UP", "down": "CART_KEY_DOWN", "left": "CART_KEY_LEFT", "right": "CART_KEY_RIGHT"}
ADD = re.compile(r"^void\s+(addGame\w*)\s*\(\s*(?:void)?\s*\)\s*\{", re.M)


def fnv1a(text):
    h = 2166136261
    for b in text.encode():
        h = ((h ^ b) * 16777619) & 0xFFFFFFFF
    return h


def table(library, carts):
    listed = {g["slug"] for g in library}
    stray = sorted(f for f in os.listdir(carts) if f.endswith(".c") and f[:-2] not in listed)
    if stray:
        sys.exit(f"carts/ has games library.json does not list: {', '.join(stray)}")
    lines = []
    for g in library:
        code = open(join(carts, g["slug"] + ".c")).read()
        add = ADD.findall(code)
        if len(add) != 1:
            sys.exit(f"{g['slug']}.c: expected one addGame...() function, found {add}")
        controls = CONTROLS[g.get("controls", "tap")]
        menu = KEYS[g.get("menu", "up")]
        lines.append(f'CART("{g["slug"]}", {json.dumps(g.get("by", g["author"]))}, 0x{fnv1a(g["slug"]):08x}u, {controls}, {menu}, {add[0]})')
    return "\n".join(lines) + "\n"


def write(path, text):
    try:
        if open(path).read() == text:
            return
    except OSError:
        pass
    open(path, "w").write(text)


def generate(out, library_path, carts):
    os.makedirs(out, exist_ok=True)
    library = json.load(open(library_path))
    write(join(out, "CartTable.inc"), table(library, carts))
    return library


if __name__ == "__main__":
    here = dirname(dirname(os.path.abspath(sys.argv[0])))
    generate(sys.argv[1], sys.argv[2] if len(sys.argv) > 2 else join(here, "library.json"),
             sys.argv[3] if len(sys.argv) > 3 else join(here, "carts"))
else:
    Import("env")  # noqa: F821
    project = env.subst("$PROJECT_DIR")  # noqa: F821
    gen = join(env.subst("$BUILD_DIR"), "carts")  # noqa: F821
    carts = join(project, "carts")
    library = generate(gen, join(project, "library.json"), carts)
    env.Append(CPPPATH=[gen, join(project, "lib", "cglp", "src")])  # noqa: F821

    # Only one game runs at a time, so their zero-initialised statics share one block of RAM: each
    # game's .bss is renamed .crisp.<slug>, and overlay.ld lays them all at the same address, after
    # the firmware's own .bss. The ESP32 has 124 KB of static RAM and the games' .bss together is
    # many times that. The host clears the block (crisp_overlay_start to _end) before a game starts.
    write(join(gen, "overlay.ld"), "SECTIONS\n{\n  OVERLAY ALIGN(8) : NOCROSSREFS\n  {\n"
          + "".join(f"    .crisp.{g['slug']} {{ *(.crisp.{g['slug']}) }}\n" for g in library)
          + "  } > dram0_0_seg\n"
          + f"  crisp_overlay_start = ADDR(.crisp.{library[0]['slug']});\n"
          + "  crisp_overlay_end = ALIGN(8);\n}\nINSERT AFTER .dram0.bss;\n")

    def add_carts(src_env, node):
        # PlatformIO's $OBJCOPY is esptool here: the toolchain's own sits beside its gcc
        objcopy = src_env.subst("$CC").replace("gcc", "objcopy")
        cart_env = src_env.Clone()
        # One .bss section a file, which objcopy can rename whole. The games hand string literals
        # to char *, as their C was written; the firmware's -Wwrite-strings would flag each one.
        cart_env.Append(CCFLAGS=["-fno-data-sections", "-fno-common"], CFLAGS=["-Wno-discarded-qualifiers"])
        objects = []
        for g in library:
            slug = g["slug"]
            raw = cart_env.Object(join(gen, slug + ".bss.o"), join(carts, slug + ".c"))
            objects += cart_env.Command(join(gen, slug + ".o"), raw,
                                        f'"{objcopy}" --rename-section .bss=.crisp.{slug} $SOURCE $TARGET')
        return [src_env.Object(node)] + objects

    env.AddBuildMiddleware(add_carts, "*/play/Carts.c")  # noqa: F821
    print(f"crisp: {len(library)} games")
