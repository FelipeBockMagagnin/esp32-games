#!/usr/bin/env python3
"""Builds the game's carrier board with KiCad's pcbnew API.

Places the parts, assigns the nets, routes through Freerouting, pours ground on both
layers and saves esp32-idle-game.kicad_pcb. Run it with the system python that has
pcbnew (KiCad 10):

    python3 hardware/generate_pcb.py --freerouting /path/to/freerouting.jar

Without --freerouting the board is saved unrouted, with ratsnest only.
"""

import argparse
import os
import subprocess
import sys

import pcbnew

HERE = os.path.dirname(os.path.abspath(__file__))
FP_ROOT = "/usr/share/kicad/footprints"
CORNER_R = 3.0
PITCH = 2.54

mm = pcbnew.FromMM


def pt(x, y):
    return pcbnew.VECTOR2I(mm(x), mm(y))


# The 2.8" ILI9341 module's 14-pin header in the order it reads with the module face up
# and its header along the top edge. Here the header is along the bottom edge, so on the
# board pin 1 (VCC) is the rightmost. The touch pins stay unconnected.
DISPLAY_PINS = ["VCC", "GND", "CS", "RESET", "DC", "MOSI", "SCK", "LED", "MISO",
                "T_CLK", "T_CS", "T_DIN", "T_DO", "T_IRQ"]

# DOIT ESP32 DevKit V1 (30 pins), mounted on the back lying on its side with its
# components facing out and the USB port towards the left edge (as seen from the front).
# Each row reads left to right from the front: ESP_LEFT is the upper row.
ESP_LEFT = ["3V3", "GND", "D15", "D2", "D4", "RX2", "TX2", "D5", "D18", "D19", "D21",
            "RX0", "TX0", "D22", "D23"]
ESP_RIGHT = ["VIN", "GND", "D13", "D12", "D14", "D27", "D26", "D25", "D33", "D32", "D35",
             "D34", "VN", "VP", "EN"]

# Pin -> net. Mirrors src/main.cpp and the TFT_eSPI flags in platformio.ini.
ESP_NETS = {
    "GND": "GND", "VIN": "+5V", "EN": "EN",
    "D15": "TFT_CS", "D2": "TFT_DC", "D13": "TFT_MOSI", "D14": "TFT_SCK",
    "D21": "TFT_LED", "D12": "TFT_MISO",
    "D23": "BTN_MENU", "D22": "BTN_DOWN", "D19": "BTN_UP", "D27": "BTN_CONFIRM",
    "D26": "BTN_BACK", "D32": "BUZZER",
}
DISPLAY_NETS = {
    "VCC": "+5V", "GND": "GND", "CS": "TFT_CS", "RESET": "EN", "DC": "TFT_DC",
    "MOSI": "TFT_MOSI", "SCK": "TFT_SCK", "LED": "TFT_LED", "MISO": "TFT_MISO",
}

# Handheld: the 2.8" display on top, the D-pad under it, the ESP32 behind the display.
# The display does not fit on a 100 mm board above the D-pad, so its header sits along
# the module's bottom edge, just above the D-pad, and its top ~17 mm overhangs the board,
# carried by the case. That keeps the board in the cheapest fab tier. The ESP32 lies on
# its side so its USB port reaches the case's side wall instead of hiding under the
# overhang. The modules are soldered straight in (2.5 mm header spacers), which keeps
# the case thin, so the buzzer driver sits on the back below the ESP32.
# 62 mm wide leaves the top screws clear of the 50 mm wide module.
# Coordinates are KiCad's (y down), in mm; pad 1 positions, and back footprints mirror,
# so their other pads go to -x.
BOARD = dict(
    name="esp32-idle-game", version="v5.0",
    board=(62.0, 100.0),
    display_header_y=66.3,  # Module's bottom edge at ~68.8, clear of the UP cap
    esp_row_y=7.8, esp_x0=8.5,  # Upper row, and its pin nearest the USB end
    dpad=(84.8, 10.5), display_text_y=19.0,
    holes=[(3.0, 3.0), (59.0, 3.0), (3.5, 96.5), (58.5, 96.5)],
    buzzer=(9.0, 50.0), r1=(40.0, 45.5), q1=(26.0, 46.0), d1=(40.0, 51.5),
    c1=(52.0, 48.0), title_y=59.0,
)
BUZZER_D = 9.0      # 9x5.5 mm passive buzzer
BUZZER_PITCH = 4.0
C1_LYING = (11.0, 5.0)  # 5x11 mm capacitor bent flat, body towards the D-pad

POWER_NETS = ["+5V", "GND", "BUZ_N"]


class Builder:
    def __init__(self, w, h):
        self.w, self.h = w, h
        self.board = pcbnew.BOARD()
        self.nets = {}

    def net(self, name):
        if name not in self.nets:
            n = pcbnew.NETINFO_ITEM(self.board, name)
            self.board.Add(n)
            self.nets[name] = n
        return self.nets[name]

    def footprint(self, lib, name, ref, value, x, y, angle=0.0, pads=None, back=False):
        fp = pcbnew.FootprintLoad(os.path.join(FP_ROOT, lib + ".pretty"), name)
        fp.SetReference(ref)
        fp.SetValue(value)
        fp.Value().SetVisible(False)
        self.board.Add(fp)
        fp.SetPosition(pt(x, y))
        fp.SetOrientationDegrees(angle)
        if back:
            # Mirrors around the origin, so pad 1 stays at (x, y) and the rest go to -x
            fp.Flip(fp.GetPosition(), pcbnew.FLIP_DIRECTION_LEFT_RIGHT)
        for number, net in (pads or {}).items():
            for pad in fp.Pads():
                if pad.GetNumber() == str(number):
                    pad.SetNet(self.net(net))
        return fp

    def text(self, s, x, y, size=1.0, layer=pcbnew.F_SilkS, angle=0.0,
             justify=pcbnew.GR_TEXT_H_ALIGN_CENTER, bold=False):
        t = pcbnew.PCB_TEXT(self.board)
        t.SetText(s)
        t.SetLayer(layer)
        t.SetPosition(pt(x, y))
        t.SetTextSize(pcbnew.VECTOR2I(mm(size), mm(size)))
        t.SetTextThickness(mm(size * (0.2 if bold else 0.15)))
        t.SetTextAngleDegrees(angle)
        t.SetHorizJustify(justify)
        if layer == pcbnew.B_SilkS:
            t.SetMirrored(True)
        self.board.Add(t)
        return t

    def rect(self, x0, y0, x1, y1, layer):
        s = pcbnew.PCB_SHAPE(self.board)
        s.SetShape(pcbnew.SHAPE_T_RECT)
        s.SetStart(pt(x0, y0))
        s.SetEnd(pt(x1, y1))
        s.SetLayer(layer)
        s.SetWidth(mm(0.15))
        self.board.Add(s)

    def buzzer(self, ref, cx, cy, pads):
        # The library has no small round buzzer, so this one is drawn here: two pads
        # BUZZER_PITCH apart (pad 1, +, square) inside a BUZZER_D outline, on the back
        fp = pcbnew.FOOTPRINT(self.board)
        fp.SetFPID(pcbnew.LIB_ID("", "Buzzer_D9mm_P4mm"))  # Board-local, no library
        fp.SetReference(ref)
        fp.SetValue("Passive buzzer 9mm")
        fp.Value().SetVisible(False)
        fp.Reference().SetFPRelativePosition(pt(0, -BUZZER_D / 2 - 1.2))
        for number, dx, shape in [(1, BUZZER_PITCH / 2, pcbnew.PAD_SHAPE_RECT),
                                  (2, -BUZZER_PITCH / 2, pcbnew.PAD_SHAPE_CIRCLE)]:
            pad = pcbnew.PAD(fp)
            pad.SetNumber(str(number))
            pad.SetAttribute(pcbnew.PAD_ATTRIB_PTH)
            pad.SetShape(shape)
            pad.SetSize(pcbnew.VECTOR2I(mm(1.8), mm(1.8)))
            pad.SetDrillSize(pcbnew.VECTOR2I(mm(1.0), mm(1.0)))
            pad.SetLayerSet(pad.PTHMask())
            pad.SetFPRelativePosition(pt(dx, 0))
            fp.Add(pad)
        for layer, r in [(pcbnew.F_SilkS, BUZZER_D / 2), (pcbnew.F_CrtYd, BUZZER_D / 2 + 0.3)]:
            c = pcbnew.PCB_SHAPE(fp)
            c.SetShape(pcbnew.SHAPE_T_CIRCLE)
            c.SetCenter(pt(0, 0))
            c.SetEnd(pt(r, 0))
            c.SetLayer(layer)
            c.SetWidth(mm(0.12 if layer == pcbnew.F_SilkS else 0.05))
            fp.Add(c)
        plus = pcbnew.PCB_TEXT(fp)
        plus.SetText("+")
        plus.SetLayer(pcbnew.F_SilkS)
        plus.SetFPRelativePosition(pt(BUZZER_PITCH / 2, -2.2))
        plus.SetTextSize(pcbnew.VECTOR2I(mm(1), mm(1)))
        fp.Add(plus)
        self.board.Add(fp)
        fp.SetPosition(pt(cx, cy))
        fp.Flip(fp.GetPosition(), pcbnew.FLIP_DIRECTION_LEFT_RIGHT)
        for pad in fp.Pads():
            pad.SetNet(self.net(pads[int(pad.GetNumber())]))
        return fp

    def outline(self):
        w, h, r = self.w, self.h, CORNER_R
        segs = [((r, 0), (w - r, 0)), ((w, r), (w, h - r)),
                ((w - r, h), (r, h)), ((0, h - r), (0, r))]
        for a, b in segs:
            s = pcbnew.PCB_SHAPE(self.board)
            s.SetShape(pcbnew.SHAPE_T_SEGMENT)
            s.SetStart(pt(*a))
            s.SetEnd(pt(*b))
            s.SetLayer(pcbnew.Edge_Cuts)
            s.SetWidth(mm(0.1))
            self.board.Add(s)
        for cx, cy, sx, sy in [(r, r, 0, r), (w - r, r, w - r, 0),
                               (w - r, h - r, w, h - r), (r, h - r, r, h)]:
            s = pcbnew.PCB_SHAPE(self.board)
            s.SetShape(pcbnew.SHAPE_T_ARC)
            s.SetCenter(pt(cx, cy))
            s.SetStart(pt(sx, sy))
            s.SetArcAngleAndEnd(pcbnew.EDA_ANGLE(90, pcbnew.DEGREES_T), False)
            s.SetLayer(pcbnew.Edge_Cuts)
            s.SetWidth(mm(0.1))
            self.board.Add(s)

    def design_rules(self):
        bds = self.board.GetDesignSettings()
        # Comfortable for any budget fab (JLCPCB's minimum is 0.127 mm)
        bds.m_MinClearance = mm(0.2)
        bds.m_TrackMinWidth = mm(0.2)  # Freerouting necks to 0.225 between header pins
        ns = bds.m_NetSettings
        default = ns.GetDefaultNetclass()
        default.SetTrackWidth(mm(0.3))
        default.SetClearance(mm(0.25))
        default.SetViaDiameter(mm(0.8))
        default.SetViaDrill(mm(0.4))
        power = pcbnew.NETCLASS("Power")
        power.SetTrackWidth(mm(0.6))
        power.SetClearance(mm(0.25))
        power.SetViaDiameter(mm(1.0))
        power.SetViaDrill(mm(0.5))
        ns.SetNetclass("Power", power)
        for n in POWER_NETS:
            ns.SetNetclassPatternAssignment(n, "Power")
        ns.RecomputeEffectiveNetclasses()

    def edge_keepout(self):
        # Freerouting ignores KiCad's edge clearance, so a strip along each edge keeps
        # its tracks off the outline
        w, h, k = self.w, self.h, 0.8
        for x0, y0, x1, y1 in [(0, 0, w, k), (0, h - k, w, h), (0, 0, k, h), (w - k, 0, w, h)]:
            z = pcbnew.ZONE(self.board)
            z.SetIsRuleArea(True)
            z.SetDoNotAllowTracks(True)
            z.SetDoNotAllowVias(True)
            z.SetDoNotAllowPads(False)
            z.SetDoNotAllowFootprints(False)
            z.SetDoNotAllowZoneFills(False)
            z.SetLayerSet(pcbnew.LSET.AllCuMask())
            o = z.Outline()
            o.NewOutline()
            for x, y in [(x0, y0), (x1, y0), (x1, y1), (x0, y1)]:
                o.Append(mm(x), mm(y))
            self.board.Add(z)

    def ground_pour(self):
        for layer in (pcbnew.F_Cu, pcbnew.B_Cu):
            z = pcbnew.ZONE(self.board)
            z.SetLayer(layer)
            z.SetNet(self.net("GND"))
            z.SetLocalClearance(mm(0.3))
            z.SetMinThickness(mm(0.25))
            z.SetPadConnection(pcbnew.ZONE_CONNECTION_THERMAL)
            z.SetThermalReliefGap(mm(0.4))
            z.SetThermalReliefSpokeWidth(mm(0.5))
            o = z.Outline()
            o.NewOutline()
            for x, y in [(0.5, 0.5), (self.w - 0.5, 0.5),
                         (self.w - 0.5, self.h - 0.5), (0.5, self.h - 0.5)]:
                o.Append(mm(x), mm(y))
            self.board.Add(z)
        pcbnew.ZONE_FILLER(self.board).Fill(self.board.Zones())


def place(b, v):
    w, h = v["board"]
    cx = w / 2
    pins = DISPLAY_PINS
    display_right = cx + (len(pins) - 1) / 2 * PITCH  # Pin 1; the header is centred
    hy = v["display_header_y"]
    row_y, x0 = v["esp_row_y"], v["esp_x0"]

    b.outline()
    b.design_rules()

    # Front: display header just above the D-pad; the module rises from it, 2.5 mm up
    display_pads = {i + 1: DISPLAY_NETS[name]
                    for i, name in enumerate(pins) if name in DISPLAY_NETS}
    # Footprint runs downwards from pin 1, so -90 lays it out right to left
    b.footprint("Connector_PinHeader_2.54mm",
                "PinHeader_1x%02d_P2.54mm_Vertical" % len(pins),
                "J1", "ILI9341 2.8in", display_right, hy, -90, display_pads)
    for i, name in enumerate(pins):
        b.text(name, display_right - i * PITCH, hy - 2.2, 0.8, angle=90,
               justify=pcbnew.GR_TEXT_H_ALIGN_LEFT)
    b.text("DISPLAY 2.8\" ILI9341", cx, v["display_text_y"], 1.2)
    b.text("face up, header at the bottom - check VCC/GND", cx, v["display_text_y"] + 2.3, 0.9)

    # D-pad: confirm in the middle, the four directions around it. Right is MENU (next
    # screen) and left is BACK (previous screen).
    dpad_y, step = v["dpad"]
    buttons = [
        ("SW1", "UP", "BTN_UP", (cx, dpad_y - step)),
        ("SW2", "DOWN", "BTN_DOWN", (cx, dpad_y + step)),
        ("SW3", "OK", "BTN_CONFIRM", (cx, dpad_y)),
        ("SW4", "BACK", "BTN_BACK", (cx - step, dpad_y)),
        ("SW5", "MENU", "BTN_MENU", (cx + step, dpad_y)),
    ]
    for ref, label, net, (bx, by) in buttons:
        # Footprint origin is pad 1; its body is centred 3.25/2.25 mm from there
        sw = b.footprint("Button_Switch_THT", "SW_PUSH_6mm", ref, "6x6 tact",
                         bx - 3.25, by - 2.25, 0, {1: net, 2: "GND"})
        sw.Reference().SetVisible(False)  # The label takes its place
        b.text(label, bx, by - 5.2, 1.0, bold=True)

    # Back: the ESP32 on its side behind the display, its pins through the board.
    # -90 would run left; the flip to the back mirrors it to run right from x0.
    for ref, y, row, label_dy in [("J2", row_y, ESP_LEFT, -2.2),
                                  ("J3", row_y + 10 * PITCH, ESP_RIGHT, 2.2)]:
        pads = {i + 1: ESP_NETS[name] for i, name in enumerate(row) if name in ESP_NETS}
        b.footprint("Connector_PinHeader_2.54mm", "PinHeader_1x15_P2.54mm_Vertical",
                    ref, "ESP32 DevKit V1", x0, y, -90, pads, back=True)
        # Upper labels grow upwards, lower ones downwards (mirrored text, so the
        # justification reads backwards)
        just = pcbnew.GR_TEXT_H_ALIGN_RIGHT if label_dy < 0 else pcbnew.GR_TEXT_H_ALIGN_LEFT
        for i, name in enumerate(row):
            b.text(name, x0 + i * PITCH, y + label_dy, 0.8, layer=pcbnew.B_SilkS,
                   angle=90, justify=just)
    mid_y = row_y + 5 * PITCH
    b.text("USB", x0 - 5.0, mid_y, 1.2, layer=pcbnew.B_SilkS, angle=90, bold=True)
    b.text("ESP32 DevKit V1 30p", x0 + 7 * PITCH, mid_y - 1.2, 1.0, layer=pcbnew.B_SilkS)
    b.text("(components facing out)", x0 + 7 * PITCH, mid_y + 1.2, 0.8, layer=pcbnew.B_SilkS)

    # Buzzer driver, driven low-side from 5 V so it is louder than straight off a 3.3 V
    # pin. Everything here is on the back, below the ESP32, and no taller than it.
    bz, r1, q1, d1, c1 = (v[k] for k in ("buzzer", "r1", "q1", "d1", "c1"))
    b.buzzer("BZ1", bz[0], bz[1], {1: "+5V", 2: "BUZ_N"})
    b.footprint("Resistor_THT", "R_Axial_DIN0207_L6.3mm_D2.5mm_P7.62mm_Horizontal",
                "R1", "1k", r1[0], r1[1], 0, {1: "BUZZER", 2: "BUZ_B"}, back=True)
    b.footprint("Package_TO_SOT_THT", "TO-92_Inline_Wide", "Q1", "S8050 (EBC)",
                q1[0], q1[1], 0, {1: "GND", 2: "BUZ_B", 3: "BUZ_N"}, back=True)
    for i, pin in enumerate("EBC"):
        b.text(pin, q1[0] - i * PITCH, q1[1] + 2.8, 0.8, layer=pcbnew.B_SilkS)
    b.footprint("Diode_THT", "D_DO-35_SOD27_P7.62mm_Horizontal", "D1", "1N4148",
                d1[0], d1[1], 0, {1: "+5V", 2: "BUZ_N"}, back=True)
    b.footprint("Capacitor_THT", "CP_Radial_D5.0mm_P2.00mm", "C1", "100uF 16V",
                c1[0], c1[1], 0, {1: "+5V", 2: "GND"}, back=True)
    # Where the capacitor's body lies once bent flat
    length, dia = C1_LYING
    mid = c1[0] - 1.0  # Between the two pads
    top = c1[1] + 2.8   # Clear of the footprint's own outline
    b.rect(mid - dia / 2, top, mid + dia / 2, top + length, pcbnew.B_SilkS)
    b.text("C1", mid, top + length / 2, 0.8, layer=pcbnew.B_SilkS, angle=90)

    for i, (x, y) in enumerate(v["holes"]):
        b.footprint("MountingHole", "MountingHole_3.2mm_M3", "H%d" % (i + 1), "M3", x, y)

    b.text("ESP32 IDLE GAME", cx - 4, v["title_y"], 1.6, layer=pcbnew.B_SilkS, bold=True)
    b.text(v["version"], w - 7.0, dpad_y + step / 2 + 1, 1.2, layer=pcbnew.B_SilkS)
    b.edge_keepout()


def route(b, jar):
    dsn = os.path.join(HERE, "build", "board.dsn")
    ses = os.path.join(HERE, "build", "board.ses")
    os.makedirs(os.path.dirname(dsn), exist_ok=True)
    if not pcbnew.ExportSpecctraDSN(b.board, dsn):
        sys.exit("DSN export failed")
    subprocess.run(["java", "-jar", jar, "-de", dsn, "-do", ses, "-mp", "30",
                    "--gui.enabled=false"], check=True)
    if not pcbnew.ImportSpecctraSES(b.board, ses):
        sys.exit("SES import failed")


def main():
    ap = argparse.ArgumentParser()
    ap.add_argument("--freerouting", help="path to freerouting.jar")
    args = ap.parse_args()

    v = BOARD
    b = Builder(*v["board"])
    place(b, v)
    if args.freerouting:
        route(b, args.freerouting)
    b.ground_pour()
    path = os.path.join(HERE, v["name"] + ".kicad_pcb")
    b.board.Save(path)
    print("saved", path)


if __name__ == "__main__":
    main()
