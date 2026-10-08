// Just enough of the Arduino API for the game logic in src/game/ to build on a PC, so the
// balance simulator runs the real tables and formulas. The clock is driven by the sim.
#ifndef SIM_ARDUINO_H
#define SIM_ARDUINO_H

#include <cstdint>
#include <cstdio>
#include <cstdlib>
#include <cstring>

#define PROGMEM
#define pgm_read_byte(p) (*(const uint8_t *)(p))
#define pgm_read_word(p) (*(const uint16_t *)(p))

unsigned long millis();
long random(long max);
long random(long min, long max);

#endif // SIM_ARDUINO_H
