#ifndef PHASE_STYLE_H
#define PHASE_STYLE_H

#include <Arduino.h>
#include "../app/FocusTimer.h"
#include "../ui/Theme.h"

inline Color phaseColor(Phase phase)
{
    switch (phase)
    {
    case Phase::BREAK:
        return Theme::GOOD;
    case Phase::LONG_BREAK:
        return Theme::INFO;
    default:
        return Theme::BAD;
    }
}

inline const char *phaseName(Phase phase)
{
    switch (phase)
    {
    case Phase::BREAK:
        return "BREAK";
    case Phase::LONG_BREAK:
        return "LONG BREAK";
    default:
        return "FOCUS";
    }
}

// Rounded up, so the display reads 25:00 for the whole first second and never shows 00:00 running
inline void formatMinSec(unsigned long ms, char *buf, size_t size)
{
    unsigned long s = (ms + 999) / 1000;
    snprintf(buf, size, "%02lu:%02lu", s / 60, s % 60);
}

#endif // PHASE_STYLE_H
