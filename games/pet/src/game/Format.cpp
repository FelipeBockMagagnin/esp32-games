#include "Format.h"

String formatAmount(uint64_t value)
{
    static const char *const SUFFIXES[] = {"K", "M", "B", "T", "Q", "Qi"};
    char buf[16];

    if (value < 10000)
    {
        snprintf(buf, sizeof(buf), "%u", (unsigned)value);
        return String(buf);
    }

    // Keep one decimal: work in tenths of the current suffix
    uint64_t tenths = value / 100;
    uint8_t suffix = 0;
    while (tenths >= 10000 && suffix < sizeof(SUFFIXES) / sizeof(SUFFIXES[0]) - 1)
    {
        tenths /= 1000;
        suffix++;
    }

    snprintf(buf, sizeof(buf), "%u.%u%s", (unsigned)(tenths / 10), (unsigned)(tenths % 10), SUFFIXES[suffix]);
    return String(buf);
}

FormattedText formatCompact(uint32_t value)
{
    static const char SUFFIXES[] = {'K', 'M', 'B'};
    FormattedText out;
    if (value < 10000)
    {
        snprintf(out.text, sizeof(out.text), "%u", (unsigned)value);
        return out;
    }

    uint32_t tenths = value / 100;
    uint8_t suffix = 0;
    while (tenths >= 10000)
    {
        tenths /= 1000;
        suffix++;
    }
    if (tenths < 100)
    {
        snprintf(out.text, sizeof(out.text), "%u.%u%c", (unsigned)(tenths / 10), (unsigned)(tenths % 10), SUFFIXES[suffix]);
    }
    else
    {
        snprintf(out.text, sizeof(out.text), "%u%c", (unsigned)(tenths / 10), SUFFIXES[suffix]);
    }
    return out;
}
