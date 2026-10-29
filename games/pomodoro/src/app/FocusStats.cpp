#include "FocusStats.h"

void FocusStats::record(uint8_t minutes, uint32_t day)
{
    data.totalFocuses++;
    data.totalMinutes += minutes;
    sessionFocuses++;
    revision++;
    if (day == 0)
    {
        return;
    }

    if (day > data.lastDay)
    {
        uint32_t gap = day - data.lastDay;
        for (uint32_t d = 1; d <= gap && d <= DAYS; d++)
        {
            data.counts[(data.lastDay + d) % DAYS] = 0;
        }
        data.lastDay = day;
    }
    if (data.lastDay - day < DAYS)
    {
        data.counts[day % DAYS]++;
    }

    if (data.streakDay != day)
    {
        data.streak = data.streakDay + 1 == day ? data.streak + 1 : 1;
        data.streakDay = day;
    }
}

uint16_t FocusStats::getCount(uint32_t day) const
{
    if (day == 0 || day > data.lastDay || data.lastDay - day >= DAYS)
    {
        return 0;
    }
    return data.counts[day % DAYS];
}

uint16_t FocusStats::getStreak(uint32_t today) const
{
    return today != 0 && (data.streakDay == today || data.streakDay + 1 == today) ? data.streak : 0;
}
