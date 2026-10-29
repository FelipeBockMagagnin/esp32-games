#ifndef CLOCK_SCREEN_H
#define CLOCK_SCREEN_H

#include "Screen.h"
#include "../ui/Text.h"
#include "../ui/Header.h"
#include "Box.h"
#include "ProgressBar.h"
#include "../app/Clock.h"
#include "../app/FocusTimer.h"

// The time and date, and the focus timer at a glance
class ClockScreen : public Screen
{
public:
    ClockScreen(const Clock &clock, const FocusTimer &timer);

    void update(unsigned long now) override;

private:
    Header header;
    Text timeText;
    ProgressBar secondsBar;
    Text dateText;
    Box focusCard;
    Text focusPhaseText;
    Text focusTimeText;
    Text statusText;

    const Clock &clock;
    const FocusTimer &timer;
};

#endif // CLOCK_SCREEN_H
