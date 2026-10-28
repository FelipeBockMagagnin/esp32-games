#include "StatsScreen.h"

static const int16_t CARD_X = 5;
static const int16_t CARD_Y = 36;
static const int16_t CARD_W = 230;
static const int16_t CARD_H = 70;
static const int16_t CHART_Y = 132;
static const int16_t CHART_H = 112;

StatsScreen::StatsScreen(const FocusStats &stats, const Clock &clock)
    : header("Stats"),
      todayCard(CARD_X, CARD_Y, CARD_W, CARD_H, Theme::FRAME, true, Theme::CARD, true, 4),
      todayCount(CARD_X + 44, CARD_Y + CARD_H / 2, "0", Theme::GOLD, 1, MC_DATUM, false, Theme::CARD),
      todayLabel(CARD_X + 88, CARD_Y + 18, "", Theme::TEXT, 2, TL_DATUM),
      streakText(CARD_X + 88, CARD_Y + 44, "", Theme::TEXT_2, 1, TL_DATUM),
      weekTitle(10, CHART_Y - 14, "Last 7 days", Theme::TEXT_2, 1, TL_DATUM),
      week(CARD_X, CHART_Y, CARD_W, CHART_H),
      noClockText(120, CHART_Y + CHART_H / 2, "Needs the clock (WiFi)", Theme::TEXT_DIM, 1, MC_DATUM),
      totalText(120, 262, "", Theme::TEXT, 2, TC_DATUM),
      minutesText(120, 286, "", Theme::TEXT_2, 1, TC_DATUM),
      stats(stats),
      clock(clock)
{
    addElement(&header);
    setHeader(&header);
    addElement(&todayCard);
    todayCount.setFont(7);
    addElement(&todayCount, Theme::CARD);
    addElement(&todayLabel, Theme::CARD);
    addElement(&streakText, Theme::CARD);
    addElement(&weekTitle);
    addElement(&week);
    addElement(&noClockText);
    addElement(&totalText);
    addElement(&minutesText);
}

void StatsScreen::update(unsigned long now)
{
    if (!refreshDue(now))
    {
        return;
    }
    // Font 7 has digits only; a card this size holds two of them
    static const unsigned MAX_SHOWN = 99;

    uint32_t today = clock.today();
    unsigned count = today != 0 ? stats.getCount(today) : stats.getSessionFocuses();
    char buf[32];
    snprintf(buf, sizeof(buf), "%u", count < MAX_SHOWN ? count : MAX_SHOWN);
    todayCount.setText(buf);
    todayLabel.setText(count == 1 ? "focus" : "focuses");

    if (today != 0)
    {
        uint16_t streak = stats.getStreak(today);
        snprintf(buf, sizeof(buf), streak == 1 ? "today, 1-day streak" : "today, %u-day streak", (unsigned)streak);
        streakText.setText(buf);

        uint16_t counts[WeekChart::DAYS];
        for (uint8_t i = 0; i < WeekChart::DAYS; i++)
        {
            counts[i] = stats.getCount(today - (WeekChart::DAYS - 1) + i);
        }
        week.setData(counts, weekdayOf(today - (WeekChart::DAYS - 1)));
    }
    else
    {
        streakText.setText("since power-on");
    }
    week.setVisible(today != 0);
    noClockText.setVisible(today == 0);

    uint32_t total = stats.getTotalFocuses();
    snprintf(buf, sizeof(buf), total == 1 ? "1 focus in all" : "%lu focuses in all", (unsigned long)total);
    totalText.setText(buf);
    uint32_t minutes = stats.getTotalMinutes();
    snprintf(buf, sizeof(buf), "%luh %02lum focused", (unsigned long)(minutes / 60), (unsigned long)(minutes % 60));
    minutesText.setText(buf);
}
