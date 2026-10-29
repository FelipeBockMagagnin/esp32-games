#include "FocusScreen.h"
#include "PhaseStyle.h"

static const int16_t RING_CX = 120;
static const int16_t RING_CY = 150;
static const int16_t RING_RADIUS = 96;
static const int16_t RING_THICKNESS = 10;

FocusScreen::FocusScreen(FocusTimer &timer, SoundManager &sound)
    : header("Focus"),
      ring(RING_CX, RING_CY, RING_RADIUS, RING_THICKNESS),
      phaseText(RING_CX, RING_CY - 52, "FOCUS", Theme::BAD, 2, TC_DATUM),
      timeText(RING_CX, RING_CY, "25:00", Theme::TEXT, 1, MC_DATUM, false),
      stateText(RING_CX, RING_CY + 38, "", Theme::TEXT_2, 1, TC_DATUM),
      dots(RING_CX, 270, FocusTimer::FOCUSES_PER_CYCLE),
      hintText(120, 294, "Confirm start/pause", Theme::TEXT_DIM, 1, TC_DATUM),
      hintText2(120, 306, "Up restart - Down skip", Theme::TEXT_DIM, 1, TC_DATUM),
      timer(timer),
      sound(sound)
{
    addElement(&header);
    setHeader(&header);
    addElement(&ring);
    addElement(&phaseText);
    timeText.setFont(7);
    addElement(&timeText);
    addElement(&stateText);
    addElement(&dots);
    addElement(&hintText);
    addElement(&hintText2);
}

void FocusScreen::update(unsigned long now)
{
    if (!refreshDue(now))
    {
        return;
    }
    Phase phase = timer.getPhase();
    Color color = phaseColor(phase);
    unsigned long remaining = timer.getRemainingMs(now);
    unsigned long total = timer.getPhaseMs();

    ring.setRingColor(color);
    ring.setRemaining(total > 0 ? (uint16_t)((uint64_t)remaining * 1000 / total) : 0);
    phaseText.setText(phaseName(phase));
    phaseText.setColor(color);

    char buf[8];
    formatMinSec(remaining, buf, sizeof(buf));
    timeText.setText(buf);

    switch (timer.getState())
    {
    case TimerState::IDLE:
        stateText.setText("Press to start");
        stateText.setColor(Theme::TEXT_2);
        break;
    case TimerState::RUNNING:
        stateText.setText("");
        break;
    case TimerState::PAUSED:
        stateText.setText("PAUSED");
        stateText.setColor(Theme::GOLD);
        break;
    }

    dots.setFilled(timer.getFocusesInCycle());
    dots.setColor(phaseColor(Phase::FOCUS));
}

void FocusScreen::onConfirmPress()
{
    timer.toggle(millis());
    if (timer.getState() == TimerState::RUNNING)
    {
        sound.playStart();
    }
    else
    {
        sound.playPause();
    }
    forceRefresh();
}

void FocusScreen::onUpPress()
{
    timer.restart();
    sound.playMenu();
    forceRefresh();
}

void FocusScreen::onDownPress()
{
    timer.skip(millis());
    sound.playMenu();
    forceRefresh();
}
