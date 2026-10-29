// Just enough of the Arduino API for the game, the art and the screens to build on a PC, so
// the tests, the simulator and the screenshots run the real code. The clock is the tool's.
#ifndef STUB_ARDUINO_H
#define STUB_ARDUINO_H

#include <cmath>
#include <cstddef>
#include <cstdint>
#include <cstdio>
#include <cstdlib>
#include <cstring>
#include <string>

#define PROGMEM
#define pgm_read_byte(p) (*(const uint8_t *)(p))
#define pgm_read_word(p) (*(const uint16_t *)(p))
#define memcpy_P memcpy

#define HIGH 1
#define LOW 0

inline unsigned long stubNow = 0;
inline unsigned long millis() { return stubNow; }
inline unsigned long micros() { return stubNow * 1000; }
inline long random(long max) { return max > 0 ? rand() % max : 0; }
inline long random(long min, long max) { return min + random(max - min); }
inline void tone(int, unsigned int, unsigned long = 0) {}

struct StubSerial
{
    void println(const char *) {}
};
inline StubSerial Serial;

#endif // STUB_ARDUINO_H
