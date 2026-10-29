#!/usr/bin/env python3
"""Refreshes the board data inside viewer.html from esp32-idle-game.kicad_pcb.

viewer.html is a standalone 3D view of how the modules, parts and screws stack on the
board. Its component sizes mirror case/case.scad; the pads, tracks and holes come from
the .kicad_pcb, so run this after regenerating the board:

    python3 hardware/viewer.py

Plain python, no KiCad needed: it reads the .kicad_pcb as text.
"""

import json
import math
import os
import re

HERE = os.path.dirname(os.path.abspath(__file__))
PCB = os.path.join(HERE, "esp32-idle-game.kicad_pcb")
HTML = os.path.join(HERE, "viewer.html")
BEGIN, END = "/*PCB_DATA*/", "/*END_PCB_DATA*/"


def parse(text):
    tokens = re.findall(r'\(|\)|"(?:[^"\\]|\\.)*"|[^\s()]+', text)
    stack = [[]]
    for t in tokens:
        if t == "(":
            stack.append([])
        elif t == ")":
            node = stack.pop()
            stack[-1].append(node)
        else:
            stack[-1].append(t[1:-1] if t.startswith('"') else t)
    return stack[0][0]


def kids(node, name):
    return [c for c in node if isinstance(c, list) and c and c[0] == name]


def first(node, name):
    found = kids(node, name)
    return found[0] if found else None


def board_data(tree):
    pads = []
    for fp in kids(tree, "footprint"):
        at = first(fp, "at")
        fx, fy = float(at[1]), float(at[2])
        angle = math.radians(float(at[3]) if len(at) > 3 else 0.0)
        ref = next(p[2] for p in kids(fp, "property") if p[1] == "Reference")
        for pad in kids(fp, "pad"):
            pa = first(pad, "at")
            px, py = float(pa[1]), float(pa[2])
            # KiCad rotates counter-clockwise on screen, with y pointing down
            x = fx + px * math.cos(angle) + py * math.sin(angle)
            y = fy - px * math.sin(angle) + py * math.cos(angle)
            net = first(pad, "net")
            drill = first(pad, "drill")
            pads.append({
                "ref": ref, "num": pad[1], "x": round(x, 3), "y": round(y, 3),
                "net": net[1] if net else "", "shape": pad[3],
                "size": float(first(pad, "size")[1]),
                # (drill d) or, for a slot, (drill oval x y): the narrower side is the hole
                "drill": (min(float(drill[2]), float(drill[3])) if drill[1] == "oval"
                          else float(drill[1])) if drill else 0.0,
                "npth": pad[2] == "np_thru_hole",
            })
    segs = []
    for s in kids(tree, "segment"):
        a, b = first(s, "start"), first(s, "end")
        segs.append([round(float(a[1]), 3), round(float(a[2]), 3),
                     round(float(b[1]), 3), round(float(b[2]), 3),
                     float(first(s, "width")[1]), first(s, "layer")[1] == "F.Cu",
                     first(s, "net")[1]])
    vias = [[float(first(v, "at")[1]), float(first(v, "at")[2]),
             float(first(v, "size")[1]), float(first(v, "drill")[1])]
            for v in kids(tree, "via")]
    return {"pads": pads, "segs": segs, "vias": vias}


def main():
    with open(PCB) as f:
        data = board_data(parse(f.read()))
    with open(HTML) as f:
        html = f.read()
    start, stop = html.index(BEGIN) + len(BEGIN), html.index(END)
    html = html[:start] + json.dumps(data, separators=(",", ":")) + html[stop:]
    with open(HTML, "w") as f:
        f.write(html)
    print("updated", HTML, "-", len(data["pads"]), "pads,", len(data["segs"]), "tracks")


if __name__ == "__main__":
    main()
