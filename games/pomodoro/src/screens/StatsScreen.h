#ifndef STATS_SCREEN_H
#define STATS_SCREEN_H

#include "Screen.h"
#include "../ui/Text.h"
#include "../ui/Header.h"
#include "Box.h"
#include "../ui/WeekChart.h"
#include "../app/Clock.h"
#include "../app/FocusStats.h"

class StatsScreen : public Screen
{
public:
    StatsScreen(const FocusStats &stats, const Clock &clock);

    void update(unsigned long now) override;

private:
    Header header;
    Box todayCard;
    Text todayCount;
    Text todayLabel;
    Text streakText;
    Text weekTitle;
    WeekChart week;
    Text noClockText;
    Text totalText;
    Text minutesText;

    const FocusStats &stats;
    const Clock &clock;
};

#endif // STATS_SCREEN_H
