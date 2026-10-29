// Just enough of the Arduino API for the logic and the screens to build on a PC, so the tests
// and tools/shots.cpp run the real code. The clock is the tool's.
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

#define HIGH 1
#define LOW 0

inline unsigned long stubNow = 0;
inline unsigned long millis() { return stubNow; }
inline unsigned long micros() { return stubNow * 1000; }
inline uint32_t esp_random() { return (uint32_t)rand(); }
long random(long max);
long random(long min, long max);
inline void tone(int, unsigned int, unsigned long = 0) {}

class String
{
public:
    String(const char *text = "") : text(text != nullptr ? text : "") {}
    String(const std::string &text) : text(text) {}

    const char *c_str() const { return text.c_str(); }
    unsigned length() const { return (unsigned)text.size(); }
    bool operator==(const String &other) const { return text == other.text; }
    bool operator!=(const String &other) const { return text != other.text; }
    bool operator==(const char *other) const { return text == (other != nullptr ? other : ""); }
    bool operator!=(const char *other) const { return !(*this == other); }

private:
    std::string text;
};

struct StubSerial
{
    void println(const char *) {}
};
inline StubSerial Serial;

#endif // STUB_ARDUINO_H
