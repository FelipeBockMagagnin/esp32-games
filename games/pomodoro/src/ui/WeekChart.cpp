#include "WeekChart.h"

static const char *const WEEKDAYS = "SMTWTFS";
static const int16_t LABEL_H = 12; // A size-1 line and its gap, above and below the bars
static const int16_t BAR_W = 18;

void WeekChart::setData(const uint16_t (&newCounts)[DAYS], uint8_t weekday)
{
    if (memcmp(counts, newCounts, sizeof(counts)) == 0 && firstWeekday == weekday)
    {
        return;
    }
    memcpy(counts, newCounts, sizeof(counts));
    firstWeekday = weekday;
    markDirty();
}

void WeekChart::draw(TFT_eSPI &tft)
{
    tft.fillRect(x, y, w, h, eraseColor);

    uint16_t top = 4; // A quiet week still leaves room above its bars
    for (uint8_t i = 0; i < DAYS; i++)
    {
        top = counts[i] > top ? counts[i] : top;
    }

    int16_t pitch = w / DAYS;
    int16_t baseY = y + h - LABEL_H;
    int16_t barSpace = h - 2 * LABEL_H;
    tft.setTextSize(1);
    tft.setTextDatum(TC_DATUM);
    for (uint8_t i = 0; i < DAYS; i++)
    {
        int16_t cx = x + i * pitch + pitch / 2;
        bool today = i == DAYS - 1;
        int16_t barH = (int16_t)((int32_t)counts[i] * barSpace / top);
        if (barH > 0)
        {
            tft.fillRect(cx - BAR_W / 2, baseY - barH, BAR_W, barH, today ? Theme::GOLD_BAR : Theme::XP_BAR);
        }
        tft.drawFastHLine(cx - BAR_W / 2, baseY, BAR_W, Theme::FRAME);

        if (counts[i] > 0)
        {
            char n[6];
            snprintf(n, sizeof(n), "%u", (unsigned)counts[i]);
            tft.setTextColor(today ? Theme::GOLD : Theme::TEXT_2);
            tft.drawString(n, cx, baseY - barH - LABEL_H + 2);
        }
        char day[2] = {WEEKDAYS[(firstWeekday + i) % DAYS], '\0'};
        tft.setTextColor(today ? Theme::GOLD : Theme::TEXT_DIM);
        tft.drawString(day, cx, baseY + 4);
    }
    tft.setTextDatum(TL_DATUM);
}
