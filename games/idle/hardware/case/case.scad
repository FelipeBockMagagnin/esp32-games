// Case for the ESP32 Idle Game v5 board (62x100 mm) and its 2.8" display, whose top
// overhangs the board and rests on rails in the tray.
//
// Three printed parts:
//   bottom - tray the PCB screws into; holds the ESP32 hanging off the back
//   top    - bezel with the display window and the D-pad holes; snaps onto the tray
//   caps   - five button plungers that bridge the gap from the switches to the bezel
//
// Export one part at a time:
//   openscad -D 'part="bottom"' -o case_bottom.stl case.scad
//
// Coordinates: the PCB's front faces +Z, its bottom face is z = 0, and KiCad's y axis
// (downwards) is flipped so the board's top edge (and the display overhang) is at +Y.

part = "assembly"; // bottom | top | caps | assembly

/* [Board] */
pcb_w = 62;
pcb_h = 100;
pcb_t = 1.6;
pcb_corner = 3;
holes = [[3, 3], [59, 3], [3.5, 96.5], [58.5, 96.5]]; // KiCad coordinates

/* [Stack heights, measured from the PCB faces] */
// The modules are soldered straight in, so they sit on their male headers' plastic spacer
module_gap = 2.5;
// Display module (PCB + LCD glass) as sold: 2.8" ILI9341, ~4.5 mm
display_thick = 4.5;
// From the ESP32's PCB to its tallest part (WROOM shield, USB connector)
esp_parts = 3.5;
// Height of the tactile switch from the PCB to the top of its stem (6x6x5 -> 5)
switch_h = 5;

/* [Display] */
display_size = [50, 86];
// Centre of the module in KiCad coordinates: its header is at y = 66.3, ~2.5 mm in from
// its bottom edge, so it reaches y = -17.2, past the board's top edge
display_center = [31, 68.8 - 86 / 2];
window = [46, 62]; // A little larger than the 43.2x57.6 visible area
overhang = display_size[1] / 2 - display_center[1]; // Beyond the board's top edge

// KiCad coordinates of the 9 mm buzzer, on the back below the ESP32
buzzer_pos = [9, 50];

/* [Buttons] D-pad centre and spacing, as in generate_pcb.py */
dpad_y = 84.8;
dpad_step = 10.5;
dpad_x = pcb_w / 2;
buttons = [[dpad_x, dpad_y - dpad_step], [dpad_x, dpad_y + dpad_step], [dpad_x, dpad_y],
           [dpad_x - dpad_step, dpad_y], [dpad_x + dpad_step, dpad_y]];

/* [Case] */
wall = 2;
floor_t = 1.6;
plate_t = 1.6;
gap = 0.5;        // PCB to wall
fit = 0.15;       // Clearance per side on the snap lip
lip_h = 3;
lip_t = 1;
tray_top = pcb_t + 2.5; // Tray wall height above the PCB's bottom face

// M3 screws from the front into the tray's bosses; 2.6 suits self-tapping into PLA,
// 4.0 suits a heat-set insert
boss_hole = 2.6;
boss_d = 6.5;
rail_w = 3;

// USB cut in the left wall, centred on the ESP32's connector (KiCad y of the module's
// middle: upper pin row at 7.8, rows 25.4 apart)
usb_y = 7.8 + 12.7;
usb_size = [12, 8];

$fn = 48;

// Derived heights
esp_bottom = -(module_gap + pcb_t + esp_parts);   // Lowest point of the ESP32
floor_top = esp_bottom - 1.2;
floor_bot = floor_top - floor_t;
display_top = pcb_t + module_gap + display_thick;
plate_bot = display_top + 0.2;
plate_top = plate_bot + plate_t;
switch_top = pcb_t + switch_h;
usb_z = -(module_gap + pcb_t + 1.6);              // Middle of the connector

function ky(p) = [p[0], pcb_h - p[1]];

module rrect(w, h, r) {
    translate([r, r]) offset(r = r) square([w - 2 * r, h - 2 * r]);
}

case_len = pcb_h + overhang; // Inside length the case has to hold, PCB plus overhang

// Outline of the board plus the display overhang, grown by d on every side
module outline(d) {
    translate([-d, -d]) rrect(pcb_w + 2 * d, case_len + 2 * d, pcb_corner + d);
}

module pcb_outline() { rrect(pcb_w, pcb_h, pcb_corner); }

module bottom() {
    outer = gap + wall;
    difference() {
        union() {
            // Floor and walls up to just above the PCB
            translate([0, 0, floor_bot]) linear_extrude(tray_top - floor_bot)
                difference() { outline(outer); outline(gap); }
            translate([0, 0, floor_bot]) linear_extrude(floor_t) outline(outer);
            // Snap lip: the inner part of the wall carries on up into the bezel
            translate([0, 0, tray_top]) linear_extrude(lip_h)
                difference() { outline(gap + lip_t); outline(gap); }
            // Bumps on the long sides of the lip that click into the bezel's groove
            for (x = [-gap - lip_t, pcb_w + gap + lip_t])
                translate([x, case_len / 2, tray_top + lip_h / 2])
                    rotate([90, 0, 0]) cylinder(h = 30, r = 0.45, center = true);
            // Bosses the PCB rests on and screws into
            for (h = holes)
                translate([each ky(h), floor_top]) cylinder(h = -floor_top, d = boss_d);
            // Rails under the long edges of the display where it overhangs the board,
            // clear of the SD card slot in the middle of the module's underside
            for (x = [display_center[0] - display_size[0] / 2,
                      display_center[0] + display_size[0] / 2 - rail_w])
                translate([x, pcb_h + 0.5, floor_top])
                    cube([rail_w, overhang - 0.5, pcb_t + module_gap - floor_top]);
        }
        for (h = holes)
            translate([each ky(h), floor_top + 1]) cylinder(h = 40, d = boss_hole);
        // USB opening through the left wall
        // The plug's body reaches below the floor's top; leave a strip of floor under it
        usb_z0 = max(usb_z - usb_size[1] / 2, floor_bot + 0.6);
        translate([-10, ky([0, usb_y])[1] - usb_size[0] / 2, usb_z0])
            cube([10, usb_size[0], usb_z + usb_size[1] / 2 - usb_z0]);
        // Grille under the buzzer, which faces the floor
        for (i = [-1:1], j = [-1:1])
            translate([buzzer_pos[0] + i * 3.5, ky(buzzer_pos)[1] + j * 3.5, floor_bot - 1])
                cylinder(h = floor_t + 2, d = 2.2);
    }
}

module top() {
    outer = gap + wall;
    difference() {
        union() {
            translate([0, 0, plate_bot]) linear_extrude(plate_t) outline(outer);
            translate([0, 0, tray_top]) linear_extrude(plate_bot - tray_top)
                difference() { outline(outer); outline(gap); }
        }
        // Room for the tray's lip, with its groove for the bumps
        translate([0, 0, tray_top - 0.01]) linear_extrude(lip_h + fit)
            outline(gap + lip_t + fit);
        for (x = [-gap - lip_t - fit, pcb_w + gap + lip_t + fit])
            translate([x, case_len / 2, tray_top + lip_h / 2])
                rotate([90, 0, 0]) cylinder(h = 31, r = 0.55, center = true);
        // Display window, chamfered so the edge does not shade the screen
        translate([each ky(display_center), plate_bot - 0.01])
            linear_extrude(plate_t + 0.02, scale = [(window[0] + 2 * plate_t) / window[0],
                                                     (window[1] + 2 * plate_t) / window[1]])
                square(window, center = true);
        for (b = buttons)
            translate([each ky(b), plate_bot - 1]) cylinder(h = plate_t + 2, d = cap_d + 0.6);
        // Finger notch on one long side for prying the bezel off
        translate([pcb_w + gap + wall / 2, case_len / 2, tray_top]) cube([wall + 2, 12, 1.6], center = true);
    }
}

// Button plunger: a wide base that rests on the switch and is held under the bezel,
// and a stem through the bezel. Released, the base sits against the bezel.
// As big as the 10.5 mm spacing allows while leaving ~1.4 mm of bezel between holes;
// the bases clear each other, the display above and the wall below
cap_d = 8.5;
base_d = 10;
cap_proud = 1.5; // How far the cap stands above the bezel

module cap() {
    base_h = plate_bot - switch_top;
    cylinder(h = base_h, d = base_d);
    translate([0, 0, base_h]) cylinder(h = plate_t + cap_proud, d = cap_d);
    translate([0, 0, base_h + plate_t + cap_proud])
        scale([1, 1, 0.35]) sphere(d = cap_d); // Domed top
}

module caps() {
    // Base down: prints without supports
    for (i = [0:4]) translate([i * 12, 0, 0]) cap();
}

// Rough stand-ins for checking the fit in the assembly view
module ghosts() {
    color("darkgreen") linear_extrude(pcb_t) pcb_outline();
    color("black") translate([each ky(display_center), pcb_t + module_gap])
        translate(-display_size / 2) cube([each display_size, display_thick]);
    color("slategray") esp_pcb();
    for (b = buttons) color("dimgray") translate([each ky(b), pcb_t])
        translate([-3, -3, 0]) cube([6, 6, switch_h]);
}

include <components.scad>

if (part == "bottom") bottom();
else if (part == "top") top();
else if (part == "caps") caps();
// Parts for the 3D viewer, in their assembled positions
else if (part == "caps_placed") for (b = buttons) translate([each ky(b), switch_top]) cap();
else if (part == "header_spacers") header_spacers();
else if (part == "switch_bodies") switch_bodies();
else if (part == "switch_stems") switch_stems();
else if (part == "display_pcb") display_pcb();
else if (part == "display_glass") display_glass();
else if (part == "esp_pcb") esp_pcb();
else if (part == "esp_shield") esp_shield();
else if (part == "esp_parts_dark") esp_parts_dark();
else if (part == "buzzer") buzzer();
else if (part == "transistor") transistor();
else if (part == "resistor") resistor();
else if (part == "diode") diode();
else if (part == "capacitor") capacitor();
else {
    color("lightsteelblue") bottom();
    ghosts();
    color("lightsteelblue", 0.6) top();
    for (b = buttons) color("orange")
        translate([each ky(b), switch_top]) cap();
}
