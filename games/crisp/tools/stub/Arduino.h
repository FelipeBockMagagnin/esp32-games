// Just enough of the Arduino API for the host and the screens to build on a PC,
// so tools/run.cpp plays the real code. The clock and the buttons are the tool's.
#ifndef STUB_ARDUINO_H
#define STUB_ARDUINO_H

#include <algorithm>
#include <cmath>
#include <cstddef>
#include <cstdint>
#include <cstdio>
#include <cstdlib>
#include <cstring>
#include <string>
#include "pgmspace.h"

using std::max;
using std::min;

typedef bool boolean;
typedef uint8_t byte;
typedef uint16_t word;

#define HIGH 1
#define LOW 0
#define INPUT 0
#define OUTPUT 1
#define INPUT_PULLUP 2
#define DEC 10
#define HEX 16
#define OCT 8
#define BIN 2
#define PI 3.1415926535897932384626433832795
#define HALF_PI 1.5707963267948966192313216916398
#define TWO_PI 6.283185307179586476925286766559
#define DEG_TO_RAD 0.017453292519943295769236907684886
#define RAD_TO_DEG 57.295779513082320876798154814105

#define _BV(b) (1UL << (b))
#define bit(b) (1UL << (b))
#define bitRead(value, b) (((value) >> (b)) & 0x01)
#define bitSet(value, b) ((value) |= (1UL << (b)))
#define bitClear(value, b) ((value) &= ~(1UL << (b)))
#define bitWrite(value, b, v) ((v) ? bitSet(value, b) : bitClear(value, b))
#define bitToggle(value, b) ((value) ^= (1UL << (b)))
#define lowByte(w) ((uint8_t)((w) & 0xff))
#define highByte(w) ((uint8_t)((w) >> 8))
#define constrain(amt, low, high) ((amt) < (low) ? (low) : ((amt) > (high) ? (high) : (amt)))
#define sq(x) ((x) * (x))
#define radians(deg) ((deg) * DEG_TO_RAD)
#define degrees(rad) ((rad) * RAD_TO_DEG)
#define F_CPU 240000000L
#define RTC_NOINIT_ATTR

// Virtual time: every look at the clock moves it a little, so a game that spins on millis()
// still gets there; vTaskDelay() is a millisecond's wait
extern uint64_t stubMicros;
void stubStep(uint64_t us);
inline unsigned long micros()
{
    stubStep(2);
    return (unsigned long)stubMicros;
}
inline unsigned long millis()
{
    stubStep(2);
    return (unsigned long)(stubMicros / 1000);
}
inline void delayMicroseconds(unsigned int us) { stubStep(us); }
inline void delay(unsigned long ms) { stubStep(ms * 1000); }
inline void vTaskDelay(unsigned long ticks) { stubStep(ticks * 1000); }
inline void yield() {}

int digitalRead(uint8_t pin);
inline void digitalWrite(uint8_t, uint8_t) {}
inline void pinMode(uint8_t, uint8_t) {}
inline int analogRead(uint8_t) { return 0; }
inline void tone(int, unsigned int, unsigned long = 0) {}
inline void noTone(int) {}

long random(long max);
long random(long min, long max);
void randomSeed(unsigned long seed);
inline long map(long x, long inMin, long inMax, long outMin, long outMax)
{
    return (x - inMin) * (outMax - outMin) / (inMax - inMin) + outMin;
}

// The ESP32 core's stdlib_noniso
inline char *ltoa(long value, char *out, int base)
{
    char buf[8 * sizeof(long) + 2];
    char *p = buf + sizeof(buf) - 1;
    *p = '\0';
    bool negative = value < 0 && base == 10;
    unsigned long n = negative ? -(unsigned long)value : (unsigned long)value;
    do
    {
        int d = (int)(n % base);
        *--p = (char)(d < 10 ? '0' + d : 'a' + d - 10);
        n /= base;
    } while (n);
    if (negative)
    {
        *--p = '-';
    }
    return strcpy(out, p);
}
inline char *ultoa(unsigned long value, char *out, int base)
{
    char buf[8 * sizeof(long) + 1];
    char *p = buf + sizeof(buf) - 1;
    *p = '\0';
    do
    {
        int d = (int)(value % base);
        *--p = (char)(d < 10 ? '0' + d : 'a' + d - 10);
        value /= base;
    } while (value);
    return strcpy(out, p);
}
inline char *itoa(int value, char *out, int base) { return ltoa(value, out, base); }
inline char *utoa(unsigned value, char *out, int base) { return ultoa(value, out, base); }
inline char *dtostrf(double value, signed char width, unsigned char prec, char *out)
{
    sprintf(out, "%*.*f", width, prec, value);
    return out;
}

class __FlashStringHelper;
#define F(s) (reinterpret_cast<const __FlashStringHelper *>(s))
#define FPSTR(p) (reinterpret_cast<const __FlashStringHelper *>(p))

class String
{
public:
    String(const char *text = "") : text(text != nullptr ? text : "") {}
    String(const __FlashStringHelper *text) : String(reinterpret_cast<const char *>(text)) {}
    String(const std::string &text) : text(text) {}
    String(int v) : text(std::to_string(v)) {}
    String(unsigned v) : text(std::to_string(v)) {}
    String(long v) : text(std::to_string(v)) {}
    String(unsigned long v) : text(std::to_string(v)) {}

    const char *c_str() const { return text.c_str(); }
    unsigned length() const { return (unsigned)text.size(); }
    char operator[](unsigned i) const { return text[i]; }
    String operator+(const String &o) const { return String(text + o.text); }
    String &operator+=(const String &o)
    {
        text += o.text;
        return *this;
    }
    bool operator==(const String &other) const { return text == other.text; }
    bool operator!=(const String &other) const { return text != other.text; }

private:
    std::string text;
};

class Print
{
public:
    virtual ~Print() = default;
    virtual size_t write(uint8_t) = 0;
    virtual size_t write(const uint8_t *buffer, size_t size)
    {
        size_t n = 0;
        while (size--)
        {
            n += write(*buffer++);
        }
        return n;
    }
    size_t write(const char *str) { return str == nullptr ? 0 : write((const uint8_t *)str, strlen(str)); }
    size_t write(const char *buffer, size_t size) { return write((const uint8_t *)buffer, size); }

    size_t print(const __FlashStringHelper *s) { return write((const char *)s); }
    size_t print(const String &s) { return write(s.c_str()); }
    size_t print(const char *s) { return write(s); }
    size_t print(char c) { return write((uint8_t)c); }
    size_t print(unsigned char n, int base = DEC) { return printNumber(n, base); }
    size_t print(int n, int base = DEC) { return printSigned(n, base); }
    size_t print(unsigned int n, int base = DEC) { return printNumber(n, base); }
    size_t print(long n, int base = DEC) { return printSigned(n, base); }
    size_t print(unsigned long n, int base = DEC) { return printNumber(n, base); }
    size_t print(double n, int digits = 2)
    {
        char buf[48];
        snprintf(buf, sizeof(buf), "%.*f", digits, n);
        return write(buf);
    }

    size_t println() { return write("\r\n"); }
    template <typename T> size_t println(T v) { return print(v) + println(); }
    template <typename T> size_t println(T v, int f) { return print(v, f) + println(); }

    int printf(const char *format, ...);

private:
    size_t printSigned(long n, int base)
    {
        if (base == DEC && n < 0)
        {
            return write((uint8_t)'-') + printNumber((unsigned long)(-n), base);
        }
        return printNumber((unsigned long)n, base);
    }
    size_t printNumber(unsigned long n, int base)
    {
        if (base < 2)
        {
            base = 10;
        }
        char buf[8 * sizeof(long) + 1];
        char *str = &buf[sizeof(buf) - 1];
        *str = '\0';
        do
        {
            unsigned long m = n;
            n /= base;
            char c = (char)(m - base * n);
            *--str = c < 10 ? c + '0' : c + 'A' - 10;
        } while (n);
        return write(str);
    }
};

struct StubSerial : public Print
{
    void begin(unsigned long) {}
    size_t write(uint8_t) override { return 1; }
    using Print::write;
    int available() { return 0; }
    int read() { return -1; }
    operator bool() const { return true; }
};
inline StubSerial Serial;

#endif // STUB_ARDUINO_H
