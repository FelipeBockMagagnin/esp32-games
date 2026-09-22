#include "Format.h"
#include "GameConfig.h"

String formatAmount(uint64_t value)
{
    static const char SUFFIXES[] = {'K', 'M', 'B', 'T', 'Q'};
    char buf[16];

    if (value < 10000)
    {
        snprintf(buf, sizeof(buf), "%u", (unsigned)value);
        return String(buf);
    }

    // Keep one decimal: work in tenths of the current suffix
    uint64_t tenths = value / 100;
    uint8_t suffix = 0;
    while (tenths >= 10000 && suffix < sizeof(SUFFIXES) - 1)
    {
        tenths /= 1000;
        suffix++;
    }

    snprintf(buf, sizeof(buf), "%u.%u%c", (unsigned)(tenths / 10), (unsigned)(tenths % 10), SUFFIXES[suffix]);
    return String(buf);
}

// Rate number without sign or unit: "0.1", "12", "12.3K"
static String formatRateNumber(uint32_t perSecond)
{
    char number[16];
    unsigned whole = perSecond / GOLD_SCALE;
    unsigned frac = perSecond % GOLD_SCALE;

    if (frac == 0 || whole >= 10000)
    {
        return formatAmount(whole);
    }

    snprintf(number, sizeof(number), "%u.%03u", whole, frac);
    // Trim trailing zeros: "0.100" -> "0.1"
    char *end = number + strlen(number) - 1;
    while (*end == '0')
    {
        *end-- = '\0';
    }
    return String(number);
}

String formatRate(uint32_t perSecond)
{
    return String("+") + formatRateNumber(perSecond) + " gold/s";
}

String formatPerSecond(uint32_t perSecond)
{
    return String("+") + formatRateNumber(perSecond) + "/s";
}
