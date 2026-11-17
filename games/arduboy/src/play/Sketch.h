#ifndef SKETCH_H
#define SKETCH_H

#include <stdint.h>

// A game compiled into the firmware: its setup() and loop() live in a namespace of its own
// (src/sketches/<slug>.cpp). tools/sketches.py writes the table from library.json.
struct Sketch
{
    const char *slug;
    const char *title;
    const char *author;
    uint32_t id; // FNV-1a of the slug: names the game's EEPROM in NVS
    bool okIsB;  // The game's main action is B, so OK presses B and Left+Right A
    void (*setup)();
    void (*loop)();
};

extern const Sketch SKETCHES[];
extern const uint16_t SKETCH_COUNT;

#endif // SKETCH_H
