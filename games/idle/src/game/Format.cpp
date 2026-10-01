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

// Rate number without sign or unit, at one decimal place: "0.1", "1.2", "12", "12.3K"
static String formatRateNumber(uint64_t perSecond)
{
    char number[16];

    // Round to tenths first, so the decimal shown is the rounded one rather than a
    // truncation of the thousandths GOLD_SCALE actually stores
    uint64_t tenths = (perSecond + GOLD_SCALE / 20) / (GOLD_SCALE / 10);
    uint64_t whole = tenths / 10;
    unsigned tenth = (unsigned)(tenths % 10);

    if (tenth == 0 || whole >= 10000)
    {
        return formatAmount(whole);
    }

    snprintf(number, sizeof(number), "%u.%u", (unsigned)whole, tenth);
    return String(number);
}

String formatRate(uint64_t perSecond)
{
    return String("+") + formatRateNumber(perSecond) + " gold/s";
}

String formatPerSecond(uint64_t perSecond)
{
    return String("+") + formatRateNumber(perSecond) + "/s";
}
