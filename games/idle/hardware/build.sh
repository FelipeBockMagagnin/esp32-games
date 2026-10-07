#!/usr/bin/env bash
# Regenerates the board, checks it and packs the files a fab needs.
# Usage: hardware/build.sh /path/to/freerouting.jar
set -euo pipefail
cd "$(dirname "$0")"

NAME=esp32-idle-game
PCB=$NAME.kicad_pcb
FAB=fab/gerbers

python3 generate_pcb.py --freerouting "$1"

mkdir -p build
kicad-cli pcb drc --severity-all --exit-code-violations -o build/drc.rpt "$PCB"

rm -rf "$FAB" && mkdir -p "$FAB"
kicad-cli pcb export gerbers --no-protel-ext \
    --layers F.Cu,B.Cu,F.SilkS,B.SilkS,F.Mask,B.Mask,Edge.Cuts -o "$FAB/" "$PCB"
kicad-cli pcb export drill --format excellon --drill-origin absolute \
    --excellon-units mm -o "$FAB/" "$PCB"
rm -f "fab/$NAME-gerbers.zip"
(cd "$FAB" && zip -q "../$NAME-gerbers.zip" ./*)

for side in front back; do
    if [ "$side" = front ]; then layers=Edge.Cuts,F.Cu,F.SilkS; flip=; else layers=Edge.Cuts,B.Cu,B.SilkS; flip=--mirror; fi
    kicad-cli pcb export svg --mode-single $flip --layers "$layers" --fit-page-to-board \
        --exclude-drawing-sheet -o "build/$side.svg" "$PCB"
    rsvg-convert -h 1200 -b white "build/$side.svg" -o "fab/preview-$side.png"
done
echo "OK: fab/$NAME-gerbers.zip"
