// Simplified stand-ins for the board's parts, for the 3D viewer. Included by case.scad,
// so the stack heights and positions come from there. Each module is one colour group.

module at(p, z = 0) { translate([each ky(p), z]) children(); }

// Plastic spacers of the modules' male headers, soldered straight into the board:
// the display's 14 along its bottom edge, the ESP32's two rows of 15 lying across
module header_spacers() {
    at([31, 66.3], pcb_t) translate([-14 * 2.54 / 2, -1.25, 0]) cube([14 * 2.54, 2.5, module_gap]);
    for (y = [7.8, 7.8 + 25.4])
        at([8.5 + 7 * 2.54, y], -module_gap) translate([-15 * 2.54 / 2, -1.25, 0])
            cube([15 * 2.54, 2.5, module_gap]);
}

module switch_bodies() {
    for (b = buttons) at(b, pcb_t) translate([-3, -3, 0]) cube([6, 6, 3.5]);
}

module switch_stems() {
    for (b = buttons) at(b, pcb_t + 3.5) cylinder(h = switch_h - 3.5, d = 3.5);
}

// 2.8" module: red PCB, then the LCD glass with its black frame
module display_pcb() {
    at(display_center, pcb_t + module_gap) translate(-display_size / 2)
        cube([each display_size, pcb_t]);
}

module display_glass() {
    at(display_center, pcb_t + module_gap + pcb_t) translate([-display_size[0] / 2, -69.2 / 2, 0])
        cube([display_size[0], 69.2, display_thick - pcb_t]);
}

// DOIT DevKit V1 on its side: components facing away from the board, USB at the left.
// KiCad x of the USB end and y of the module's middle.
esp_usb_x = 8.5 - 7;
esp_mid_y = 7.8 + 12.7;
esp_z = -module_gap; // Face that touches the header spacers

module esp_box(x0, len, wid, z0, h) {
    // x0/len along the module from its USB end; wid centred on the middle line
    translate([esp_usb_x + x0, ky([0, esp_mid_y])[1] - wid / 2, z0]) cube([len, wid, h]);
}

module esp_pcb() {
    esp_box(0, 51.5, 28, esp_z - pcb_t, pcb_t);
}

module esp_shield() {
    // WROOM-32 metal can, at the end away from the USB port
    esp_box(51.5 - 6.5 - 17.6, 17.6, 18, esp_z - pcb_t - 3.1, 3.1);
    // Micro-USB receptacle, slightly proud of the board's end
    esp_box(-1, 6, 8, esp_z - pcb_t - 2.9, 2.9);
}

module esp_parts_dark() {
    // WROOM antenna section and the USB-serial chip
    esp_box(51.5 - 6.5, 6.5, 18, esp_z - pcb_t - 0.8, 0.8);
    esp_box(11, 5, 6, esp_z - pcb_t - 1.5, 1.5);
}

// Back of the board, in the strip below the ESP32
module buzzer() {
    at(buzzer_pos, -5.5) cylinder(h = 5.5, d = 9);
}

module transistor() {
    at([26 - 2.54, 46], -5) difference() {
        cylinder(h = 4.5, d = 4.8);
        translate([-3, 1.3, -1]) cube([6, 3, 7]);
    }
}

module resistor() {
    at([40 - 3.81, 45.5], -1.5) rotate([0, 90, 0]) cylinder(h = 6.3, d = 2.5, center = true);
}

module diode() {
    at([40 - 3.81, 51.5], -1.2) rotate([0, 90, 0]) cylinder(h = 3.8, d = 1.9, center = true);
}

// Bent flat, lying towards the D-pad
module capacitor() {
    at([51, 48 + 2.8], -2.6) rotate([90, 0, 0]) cylinder(h = 11, d = 5);
}
