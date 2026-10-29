#!/bin/sh
# Builds the screens for the PC against tools/stub/ and saves their scenes into the given folder
# (default: $TMPDIR/tilde-shots): a PNG of each, and a GIF of the animated ones (needs Pillow).
# A second argument sets a frame's length in ms (default 66), to watch it run as a slow board does.
set -e
cd "$(dirname "$0")/.."
out="${1:-${TMPDIR:-/tmp}/tilde-shots}"
rm -rf "$out"
mkdir -p "$out"
bin="${TMPDIR:-/tmp}/tilde-shots-bin"
g++ -std=c++17 -O2 -Wall -Wextra -Wno-unused-parameter -DPET_TOOLS -I tools/stub -o "$bin" tools/shots.cpp \
    src/pet/*.cpp src/art/*.cpp src/gfx/*.cpp src/screens/*.cpp src/core/ScreenManager.cpp src/core/SlotStore.cpp -lz
"$bin" "$out" ${2:-} > /dev/null
python3 - "$out" "${2:-66}" <<'PY'
import os, sys
from PIL import Image
out = sys.argv[1]
for scene in sorted(os.listdir(out)):
    folder = os.path.join(out, scene)
    if not os.path.isdir(folder):
        continue
    frames = [Image.open(os.path.join(folder, f)).convert("P", palette=Image.ADAPTIVE)
              for f in sorted(os.listdir(folder))]
    frames[0].save(folder + ".gif", save_all=True, append_images=frames[1:], duration=int(sys.argv[2]), loop=0)
    for f in os.listdir(folder):
        os.remove(os.path.join(folder, f))
    os.rmdir(folder)
PY
ls "$out"
