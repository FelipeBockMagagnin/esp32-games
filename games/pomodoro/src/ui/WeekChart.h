#ifndef UI_WEEK_CHART_H
#define UI_WEEK_CHART_H

#include "UI.h"

// Seven vertical bars, oldest on the left, each with its count above and its weekday below.
// The last bar is today and takes the accent.
class WeekChart : public UI
{
public:
    static const uint8_t DAYS = 7;

    WeekChart(int16_t x, int16_t y, int16_t w, int16_t h) : UI(x, y, w, h) {}

    // weekday of the first (oldest) bar, 0 being Sunday
    void setData(const uint16_t (&newCounts)[DAYS], uint8_t firstWeekday);

    void draw(TFT_eSPI &tft) override;

private:
    uint16_t counts[DAYS] = {};
    uint8_t firstWeekday = 0;
};

#endif // UI_WEEK_CHART_H
