#include "Format.h"
#include <math.h>
#include <stdio.h>

static const char *const SUFFIXES[] = {"", "K", "M", "B", "T", "Qa", "Qi", "Sx", "Sp", "Oc", "No", "Dc"};
static const int SUFFIX_COUNT = sizeof(SUFFIXES) / sizeof(SUFFIXES[0]);

static void format(double value, char *out, size_t size, int suffixLimit)
{
    if (!(value == value))
    {
        // The game is called NaN Idle, after all
        snprintf(out, size, "NaN");
        return;
    }
    if (value < 0)
    {
        value = 0;
    }
    if (value < 1000)
    {
        if (value < 10 && value != floor(value))
        {
            snprintf(out, size, "%.1f", floor(value * 10) / 10);
        }
        else
        {
            snprintf(out, size, "%.0f", floor(value));
        }
        return;
    }
    int group = (int)floor(log10(value) / 3);
    if (group < suffixLimit)
    {
        double scaled = value / pow(1000, group);
        // Rounded down, so a cost never reads as affordable before it is
        scaled = floor(scaled * 100) / 100;
        if (scaled >= 1000)
        {
            scaled = 999.99;
        }
        if (scaled >= 100)
        {
            snprintf(out, size, "%.0f%s", floor(scaled), SUFFIXES[group]);
        }
        else if (scaled >= 10)
        {
            snprintf(out, size, "%.1f%s", floor(scaled * 10) / 10, SUFFIXES[group]);
        }
        else
        {
            snprintf(out, size, "%.2f%s", scaled, SUFFIXES[group]);
        }
        return;
    }
    int exponent = (int)floor(log10(value));
    double mantissa = floor(value / pow(10, exponent) * 10) / 10;
    snprintf(out, size, "%.1fe%d", mantissa, exponent);
}

void formatNumber(double value, char *out, size_t size)
{
    format(value, out, size, SUFFIX_COUNT);
}

void formatCompact(double value, char *out, size_t size)
{
    // The sprite's tiny font has only K M B T, and two decimals would not fit
    if (value >= 1000 && value < 1e15)
    {
        int group = (int)floor(log10(value) / 3);
        double scaled = floor(value / pow(1000, group) * 10) / 10;
        if (scaled >= 10)
        {
            snprintf(out, size, "%.0f%s", floor(scaled), SUFFIXES[group]);
        }
        else
        {
            snprintf(out, size, "%.1f%s", scaled, SUFFIXES[group]);
        }
        return;
    }
    if (value >= 1e15)
    {
        snprintf(out, size, "%.0fe%d", floor(value / pow(10, floor(log10(value)))),
                 (int)floor(log10(value)));
        return;
    }
    snprintf(out, size, "%.0f", value < 0 ? 0 : floor(value));
}

void formatTime(double seconds, char *out, size_t size)
{
    uint32_t s = seconds < 0 ? 0 : (uint32_t)seconds;
    if (s < 60)
    {
        snprintf(out, size, "%us", (unsigned)s);
    }
    else if (s < 3600)
    {
        snprintf(out, size, "%um %02us", (unsigned)(s / 60), (unsigned)(s % 60));
    }
    else if (s < 86400)
    {
        snprintf(out, size, "%uh %02um", (unsigned)(s / 3600), (unsigned)(s / 60 % 60));
    }
    else
    {
        snprintf(out, size, "%ud %02uh", (unsigned)(s / 86400), (unsigned)(s / 3600 % 24));
    }
}
