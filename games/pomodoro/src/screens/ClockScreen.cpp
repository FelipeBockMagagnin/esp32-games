#include "ClockScreen.h"
#include "PhaseStyle.h"

static const char *const WEEKDAYS[] = {"Sun", "Mon", "Tue", "Wed", "Thu", "Fri", "Sat"};
static const char *const MONTHS[] = {"Jan", "Feb", "Mar", "Apr", "May", "Jun",
                                     "Jul", "Aug", "Sep", "Oct", "Nov", "Dec"};

static const int16_t TIME_Y = 100;
static const int16_t CARD_X = 5;
static const int16_t CARD_Y = 196;
static const int16_t CARD_W = 230;
static const int16_t CARD_H = 64;

ClockScreen::ClockScreen(const Clock &clock, const FocusTimer &timer)
    : header("Clock"),
      timeText(120, TIME_Y, "--:--", Theme::TEXT, 1, MC_DATUM, false),
      secondsBar(50, TIME_Y + 32, 140, 6, 0, 59, Theme::GOLD_BAR, Theme::GOLD_TRACK, Theme::GOLD_BORDER),
      dateText(120, TIME_Y + 50, "", Theme::TEXT_2, 2, TC_DATUM),
      focusCard(CARD_X, CARD_Y, CARD_W, CARD_H, Theme::FRAME, true, Theme::CARD, true, 4),
      focusPhaseText(CARD_X + 12, CARD_Y + CARD_H / 2, "", Theme::BAD, 2, ML_DATUM),
      focusTimeText(CARD_X + CARD_W - 12, CARD_Y + CARD_H / 2, "", Theme::TEXT, 2, MR_DATUM, false, Theme::CARD),
      statusText(120, 300, "", Theme::TEXT_DIM, 1, TC_DATUM),
      clock(clock),
      timer(timer)
{
    addElement(&header);
    setHeader(&header);
    timeText.setFont(7);
    addElement(&timeText);
    addElement(&secondsBar);
    addElement(&dateText);
    addElement(&focusCard);
    addElement(&focusPhaseText, Theme::CARD);
    addElement(&focusTimeText, Theme::CARD);
    addElement(&statusText);
}

void ClockScreen::update(unsigned long now)
{
    if (!refreshDue(now))
    {
        return;
    }

    char buf[24];
    tm local;
    if (clock.getLocal(local))
    {
        snprintf(buf, sizeof(buf), "%02d:%02d", local.tm_hour, local.tm_min);
        timeText.setText(buf);
        secondsBar.setVisible(true);
        secondsBar.setProgress(local.tm_sec, 59);
        snprintf(buf, sizeof(buf), "%s %d %s", WEEKDAYS[local.tm_wday], local.tm_mday, MONTHS[local.tm_mon]);
        dateText.setText(buf);
    }
    else
    {
        timeText.setText("--:--");
        secondsBar.setVisible(false);
        dateText.setText("");
    }

    Phase phase = timer.getPhase();
    focusPhaseText.setText(phaseName(phase));
    focusPhaseText.setColor(phaseColor(phase));
    formatMinSec(timer.getRemainingMs(now), buf, sizeof(buf));
    if (timer.getState() == TimerState::PAUSED)
    {
        strncat(buf, " ||", sizeof(buf) - strlen(buf) - 1);
    }
    focusTimeText.setText(buf);
    focusTimeText.setColor(timer.getState() == TimerState::RUNNING ? Theme::TEXT : Theme::TEXT_DIM);

    if (!clock.hasWifiConfig())
    {
        statusText.setText("No WiFi set: include/secrets.h");
    }
    else if (!clock.isConnected())
    {
        statusText.setText(clock.isSynced() ? "WiFi lost, clock still running" : "Joining WiFi...");
    }
    else
    {
        statusText.setText(clock.isSynced() ? "" : "Asking NTP for the time...");
    }
}
