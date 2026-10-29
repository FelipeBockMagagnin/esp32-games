#!/usr/bin/env python3
"""Packs the case's STLs into one case.3mf to send to a print service.

Every part is laid out on one plate in its print orientation (the bezel face down), and
each logo colour is another part of the object it inlays, so a slicer keeps it in place
and a multi-material printer prints it in its own filament.

    python3 hardware/case/export_3mf.py

Run it after exporting the STLs from case.scad. Plain python, no libraries.
"""

import os
import re
import zipfile

HERE = os.path.dirname(os.path.abspath(__file__))
OUT = os.path.join(HERE, "case.3mf")
MATERIALS = [("Case", "#F2F2F2"), ("Logo", "#1A1A1A"), ("Rosto", "#8C8C8C")]
SPACING = 8

# name, [(stl, material)], upside down
OBJECTS = [
    ("Bandeja", [("case_bottom.stl", 0), ("case_bottom_logo.stl", 1), ("case_bottom_face.stl", 2)], False),
    ("Moldura", [("case_top.stl", 0), ("case_top_logo.stl", 1)], True),
    ("Botoes", [("case_caps.stl", 0)], False),
]


def read_stl(path):
    text = open(path).read()
    coords = re.findall(r"vertex\s+(\S+)\s+(\S+)\s+(\S+)", text)
    return [tuple(float(c) for c in v) for v in coords]


def mesh(points):
    index, vertices, triangles = {}, [], []
    for i in range(0, len(points), 3):
        tri = []
        for p in points[i:i + 3]:
            if p not in index:
                index[p] = len(vertices)
                vertices.append(p)
            tri.append(index[p])
        if len(set(tri)) == 3:
            triangles.append(tri)
    return vertices, triangles


def main():
    parts = []
    x = 0
    for name, files, flip in OBJECTS:
        loaded = [(read_stl(os.path.join(HERE, f)), m) for f, m in files]
        if flip:
            loaded = [([(px, -py, -pz) for px, py, pz in pts], m) for pts, m in loaded]
        everything = [p for pts, _ in loaded for p in pts]
        lo = [min(p[i] for p in everything) for i in range(3)]
        hi = [max(p[i] for p in everything) for i in range(3)]
        shift = (x - lo[0], -lo[1], -lo[2])
        x += hi[0] - lo[0] + SPACING
        moved = [([(px + shift[0], py + shift[1], pz + shift[2]) for px, py, pz in pts], m)
                 for pts, m in loaded]
        parts.append((name, [(mesh(pts), m) for pts, m in moved]))

    xml = ['<?xml version="1.0" encoding="UTF-8"?>',
           '<model unit="millimeter" xml:lang="pt-BR" '
           'xmlns="http://schemas.microsoft.com/3dmanufacturing/core/2015/02">',
           '<metadata name="Title">ESP32 Idle Game - case dotcompany</metadata>',
           '<resources>', '<basematerials id="1">']
    xml += [f'<base name="{n}" displaycolor="{c}"/>' for n, c in MATERIALS]
    xml.append('</basematerials>')
    next_id = 2
    items = []
    for name, meshes in parts:
        ids = []
        for i, ((vertices, triangles), m) in enumerate(meshes):
            part_name = name if i == 0 else f"{name} {MATERIALS[m][0].lower()}"
            xml.append(f'<object id="{next_id}" name="{part_name}" type="model" pid="1" pindex="{m}">')
            xml.append('<mesh><vertices>')
            xml += [f'<vertex x="{a:.4f}" y="{b:.4f}" z="{c:.4f}"/>' for a, b, c in vertices]
            xml.append('</vertices><triangles>')
            xml += [f'<triangle v1="{a}" v2="{b}" v3="{c}"/>' for a, b, c in triangles]
            xml.append('</triangles></mesh></object>')
            ids.append(next_id)
            next_id += 1
        if len(ids) == 1:
            items.append(ids[0])
            continue
        xml.append(f'<object id="{next_id}" name="{name}" type="model"><components>')
        xml += [f'<component objectid="{i}"/>' for i in ids]
        xml.append('</components></object>')
        items.append(next_id)
        next_id += 1
    xml.append('</resources><build>')
    xml += [f'<item objectid="{i}"/>' for i in items]
    xml.append('</build></model>')

    with zipfile.ZipFile(OUT, "w", zipfile.ZIP_DEFLATED) as z:
        z.writestr("[Content_Types].xml",
                   '<?xml version="1.0" encoding="UTF-8"?>'
                   '<Types xmlns="http://schemas.openxmlformats.org/package/2006/content-types">'
                   '<Default Extension="rels" ContentType="application/vnd.openxmlformats-package.relationships+xml"/>'
                   '<Default Extension="model" ContentType="application/vnd.ms-package.3dmanufacturing-3dmodel+xml"/>'
                   '</Types>')
        z.writestr("_rels/.rels",
                   '<?xml version="1.0" encoding="UTF-8"?>'
                   '<Relationships xmlns="http://schemas.openxmlformats.org/package/2006/relationships">'
                   '<Relationship Target="/3D/3dmodel.model" Id="rel0" '
                   'Type="http://schemas.microsoft.com/3dmanufacturing/2013/01/3dmodel"/>'
                   '</Relationships>')
        z.writestr("3D/3dmodel.model", "\n".join(xml))
    print(f"{OUT}: plate {x - SPACING:.0f} mm wide")


if __name__ == "__main__":
    main()
