#ifndef GAME_FORMAT_H
#define GAME_FORMAT_H

#include <Arduino.h>

// "999", "12.3K", "4.5M"... fits the narrow columns of the 240px screen
String formatAmount(uint64_t value);

// A short text held by value, so refresh paths never build a String on the heap
struct FormattedText
{
    char text[24];
    const char *c_str() const { return text; }
    operator const char *() const { return text; }
};

// At most 4 chars, for the stat cards' size-2 numbers: "9999", "14K", "1.2M"
FormattedText formatCompact(uint32_t value);

#endif // GAME_FORMAT_H
