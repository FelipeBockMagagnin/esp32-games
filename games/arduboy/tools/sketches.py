# PlatformIO pre-script: builds every game of library.json as the Arduino IDE would, but inside a
# namespace of its own (ab_ and the slug, dashes as underscores), and writes the table of games
# (src/play/Sketch.h) in library.json's order, which src/play/Sketches.cpp includes. Everything
# it writes goes to the build folder's sketches/, and is compiled along with Sketches.cpp, with
# src/'s flags: tools/publish.py refuses a firmware built from untracked files under src/.
#
# A game's .ino files are joined (the one with setup() first, or the entry's `main`, then the rest
# in name order) with a prototype of every function before the first definition, as the IDE does;
# the functions are found in the preprocessor's output, so only the #if branches that count are
# read. That and each .cpp under sketches/<slug>/, but those in the folders the entry's `skip`
# names, become a source of gen/ that includes Prelude.h, opens the namespace and includes it.
import json
import os
import re
import subprocess
import unicodedata
from os.path import join

Import("env")

TEXT = 24


def fnv1a(text):
    h = 2166136261
    for b in text.encode():
        h = ((h ^ b) * 16777619) & 0xFFFFFFFF
    return h


# The board's only font has no accents
def ascii_text(text):
    plain = unicodedata.normalize("NFKD", text).encode("ascii", "ignore").decode()
    return json.dumps(plain[: TEXT - 1])


project = env.subst("$PROJECT_DIR")
library = json.load(open(join(project, "library.json")))
gen = join(env.subst("$BUILD_DIR"), "sketches")
prelude = join(project, "src", "sketches", "Prelude.h")
written = set()


def write(path, text):
    written.add(os.path.basename(path))
    try:
        if open(path).read() == text:
            return
    except OSError:
        pass
    open(path, "w").write(text)


def namespace(slug):
    return "ab_" + slug.replace("-", "_")


def wrapper(slug, included):
    return (
        f'#include "{prelude}"\n\n'
        f"namespace {namespace(slug)}\n{{\n"
        "using abcompat::abs;\nusing abcompat::max;\nusing abcompat::min;\n"
        f'#include "{included}"\n}}\n'
    )

KEYWORDS = {"if", "while", "for", "switch", "return", "else", "do", "sizeof", "catch"}
NOT_FUNCTIONS = {"struct", "class", "enum", "union", "namespace", "typedef", "using", "operator"}
DEFINITION = re.compile(
    r"(?P<template>template\s*<[^{};]*>\s*)?"
    r"(?P<head>[A-Za-z_][\w:<>,*&\s]*?[\s*&])(?P<name>[A-Za-z_]\w*)\s*"
    r"\((?P<args>[^;{}()]*(?:\([^()]*\)[^;{}()]*)*)\)\s*(?:const\s*)?\{$"
)
LINEMARKER = re.compile(r'^# (\d+) "((?:[^"\\]|\\.)*)"')


def blank_strings(code):
    """The code with strings, characters and preprocessor lines turned into spaces."""
    out = list(code)
    i, n = 0, len(code)
    while i < n:
        c = code[i]
        if c in "\"'":
            j = i + 1
            while j < n and code[j] not in (c, "\n"):
                j += 2 if code[j] == "\\" else 1
            j += 1
        elif c == "#" and (i == 0 or code[i - 1] == "\n"):
            j = code.find("\n", i)
            j = n if j < 0 else j
        else:
            i += 1
            continue
        for k in range(i, min(j, n)):
            if out[k] != "\n":
                out[k] = " "
        i = j
    return "".join(out)


def prototypes(preprocessed, inos):
    """Each function the .ino files define at file scope, as the compiler sees them (the
    preprocessor has run, so only the #if branches that count are left), and the file and line
    of the first definition."""
    origins = []  # (file, line) of each line of the preprocessed text
    current, number = "", 0
    for line in preprocessed.split("\n"):
        m = LINEMARKER.match(line)
        if m:
            current, number = m.group(2), int(m.group(1)) - 1
        origins.append((current, number))
        number += 1
    plain = blank_strings(preprocessed)
    found, first = [], None
    depth, start, row = 0, 0, 0
    for i, c in enumerate(plain):
        if c == "\n":
            row += 1
        elif c == "{":
            if depth == 0:
                chunk = plain[start : i + 1]
                m = DEFINITION.search(chunk)
                where = origins[plain.count("\n", 0, start + m.start()) if m else 0] if m else None
                if m and where[0] in inos:
                    head = " ".join(m.group("head").split())
                    name = m.group("name")
                    args = " ".join(m.group("args").split())
                    words = set(re.findall(r"\w+", head))
                    # Default arguments may be given once only, and a member defined outside its
                    # class has its declaration already
                    if name not in KEYWORDS and not words & NOT_FUNCTIONS and not head.endswith("::") and "=" not in args:
                        template = " ".join((m.group("template") or "").split())
                        found.append(f"{template} {head} {name}({args});".strip())
                        if first is None:
                            first = where
            depth += 1
        elif c == "}":
            depth -= 1
            if depth == 0:
                start = i + 1
        elif c == ";" and depth == 0:
            start = i + 1
    return found, first


def preprocess(code, slug):
    """The joined sketch after the preprocessor, as the ESP32 compiler would see it."""
    path = join(gen, slug + ".pre")
    open(path, "w").write(f'#include "{prelude}"\n' + code)
    includes = ["tools/stub", "lib/AvrShim/src", "lib/Arduboy2/src", "lib/ArduboyTones/src", "lib/FixedPoints/src", "src/sketches"]
    cmd = [env.subst("$CXX"), "-x", "c++", "-E", "-std=gnu++17", "-w", "-DARDUINO=10819", "-DESP32=1"]
    cmd += [f"-I{join(project, i)}" for i in includes] + [path]
    result = subprocess.run(cmd, env=env["ENV"], capture_output=True, text=True)
    if result.returncode != 0:
        raise SystemExit(f"Preprocessing {slug} failed:\n{result.stderr}")
    return result.stdout


def build_sketch(entry):
    slug = entry["slug"]
    folder = join(project, "sketches", slug)
    inos = sorted(f for f in os.listdir(folder) if f.endswith(".ino"))
    if not inos:
        raise SystemExit(f"sketches/{slug} has no .ino")
    # The main tab is the one with setup(), as the IDE names it after the sketch folder
    main = entry.get("main") or next(
        (f for f in inos if re.search(r"\bvoid\s+setup\s*\(", open(join(folder, f), errors="replace").read())), inos[0]
    )
    inos.remove(main)
    inos.insert(0, main)
    skip = [join(folder, s) for s in entry.get("skip", [])]
    sources = []
    for root, dirs, files in os.walk(folder):
        dirs.sort()
        if any(root == s or root.startswith(s + os.sep) for s in skip):
            continue
        sources += [join(root, f) for f in sorted(files) if f.endswith(".cpp")]
    sections = []
    for f in inos:
        path = join(folder, f)
        text = open(path, encoding="utf-8", errors="replace").read()
        # The sketch's own headers, named from its folder rather than gen/; a library's stays
        text = re.sub(
            r'^(\s*#\s*include\s*)"([^"]+)"',
            lambda m: f'{m.group(1)}"{join(folder, m.group(2))}"' if os.path.exists(join(folder, m.group(2))) else m.group(0),
            text,
            flags=re.M,
        )
        sections.append((path, text if text.endswith("\n") else text + "\n"))
    code = "".join(f'#line 1 "{p}"\n{t}' for p, t in sections)

    found, first = prototypes(preprocess(code, slug), [p for p, _ in sections])
    out = []
    for path, text in sections:
        lines = text.split("\n")
        at = first[1] - 1 if first and first[0] == path else None
        if at is None:
            out.append(f'#line 1 "{path}"\n{text}')
            continue
        out.append(f'#line 1 "{path}"\n' + "\n".join(lines[:at]) + "\n")
        out.append("\n".join(found) + f'\n#line {at + 1} "{path}"\n' + "\n".join(lines[at:]))
    write(join(gen, slug + ".ino.inc"), "".join(out))
    write(join(gen, slug + ".cpp"), wrapper(slug, slug + ".ino.inc"))
    for f in sources:
        name = os.path.relpath(f, folder).replace(os.sep, "_")
        write(join(gen, f"{slug}__{name}"), wrapper(slug, f))


os.makedirs(gen, exist_ok=True)
for g in library:
    build_sketch(g)

lines = []
for g in library:
    lines.append(f"namespace {namespace(g['slug'])} {{ void setup(); void loop(); }}")
lines += ["", "const Sketch SKETCHES[] = {"]
for g in library:
    ns = namespace(g["slug"])
    # The board's list fits 16 characters of a name: `name` and `by` shorten the long ones
    title = ascii_text(g.get("name", g["title"]))
    author = ascii_text(g.get("by", g["author"]))
    ok_is_b = "true" if g.get("ok") == "B" else "false"
    lines.append(f"    {{\"{g['slug']}\", {title}, {author}, 0x{fnv1a(g['slug']):08x}u, {ok_is_b}, {ns}::setup, {ns}::loop}},")
lines += ["};", f"const uint16_t SKETCH_COUNT = {len(library)};", ""]
write(join(gen, "SketchTable.inc"), "\n".join(lines))

# A game taken out of library.json leaves nothing behind to build
for f in os.listdir(gen):
    if f not in written and not f.endswith((".pre", ".o", ".d")):
        os.remove(join(gen, f))
env.Append(CPPPATH=[gen])


def add_games(src_env, node):
    games = [src_env.Object(join(gen, f[:-4] + ".o"), join(gen, f)) for f in sorted(written) if f.endswith(".cpp")]
    return [src_env.Object(node)] + games


env.AddBuildMiddleware(add_games, "*/play/Sketches.cpp")
print(f"Arduboy: {len(library)} games")
