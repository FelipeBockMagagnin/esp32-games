#ifndef SKETCH_PRELUDE_H
#define SKETCH_PRELUDE_H

// Every header a game includes, included here first and outside its namespace: the game's own
// #include of one is then a no-op, and only the game's code lands in the namespace
#include <Arduino.h>
#include <EEPROM.h>
#include <Print.h>
#include <SPI.h>
#include <WString.h>
#include <avr/eeprom.h>
#include <avr/interrupt.h>
#include <avr/io.h>
#include <avr/pgmspace.h>
#include <avr/power.h>
#include <avr/sleep.h>
#include <limits.h>
#include <math.h>
#include <stddef.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <Arduboy2.h>
#include <ArduboyTones.h>
#include <FixedPoints.h>
#include <FixedPointsCommon.h>
#include <ArduboyTonesPitches.h>
#include <Sprites.h>
#include <SpritesB.h>

// The AVR core's min(), max() and abs() are macros, happy with any type; the ESP32's are std's,
// which want one type, or a number. A wrapper's using-declarations hide std's in the game.
namespace abcompat
{
    template <class A, class B> constexpr auto min(A a, B b) -> decltype(a < b ? a : b)
    {
        return b < a ? b : a;
    }
    template <class A, class B> constexpr auto max(A a, B b) -> decltype(a > b ? a : b)
    {
        return a < b ? b : a;
    }
    template <class T> constexpr auto abs(T x) -> decltype(x > 0 ? x : -x)
    {
        return x > 0 ? x : -x;
    }
}

#endif
