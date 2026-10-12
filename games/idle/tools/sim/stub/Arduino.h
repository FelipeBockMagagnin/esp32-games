// Just enough of the Arduino API for the game logic in src/game/ to build on a PC, so the
// balance simulator runs the real tables and formulas. The clock is driven by the sim.
#ifndef SIM_ARDUINO_H
#define SIM_ARDUINO_H

#include <cstddef>
#include <cstdint>
#include <cstdio>
#include <cstdlib>
#include <cstring>
#include <string>

#define PROGMEM
#define pgm_read_byte(p) (*(const uint8_t *)(p))
#define pgm_read_word(p) (*(const uint16_t *)(p))

unsigned long millis();
long random(long max);
long random(long min, long max);

// What Format.cpp needs of it, which only the tests link
class String
{
public:
    String(const char *text = "") : text(text) {}
    const char *c_str() const { return text.c_str(); }

private:
    std::string text;
};

struct SimSerial
{
    void println(const char *) {}
};
inline SimSerial Serial;

#endif // SIM_ARDUINO_H
