#include "FocusTimer.h"

void FocusTimer::setDurations(uint8_t focusMin, uint8_t breakMin, uint8_t longBreakMin)
{
    minutes[(uint8_t)Phase::FOCUS] = focusMin;
    minutes[(uint8_t)Phase::BREAK] = breakMin;
    minutes[(uint8_t)Phase::LONG_BREAK] = longBreakMin;
}

unsigned long FocusTimer::getRemainingMs(unsigned long now) const
{
    switch (state)
    {
    case TimerState::RUNNING:
        return (long)(deadline - now) > 0 ? deadline - now : 0;
    case TimerState::PAUSED:
        return pausedRemaining;
    default:
        return phaseMs(phase);
    }
}

void FocusTimer::toggle(unsigned long now)
{
    switch (state)
    {
    case TimerState::IDLE:
        state = TimerState::RUNNING;
        deadline = now + phaseMs(phase);
        break;
    case TimerState::RUNNING:
        pausedRemaining = getRemainingMs(now);
        state = TimerState::PAUSED;
        break;
    case TimerState::PAUSED:
        state = TimerState::RUNNING;
        deadline = now + pausedRemaining;
        break;
    }
}

void FocusTimer::restart()
{
    state = TimerState::IDLE;
}

void FocusTimer::enter(Phase next, unsigned long now, bool run)
{
    phase = next;
    state = run ? TimerState::RUNNING : TimerState::IDLE;
    deadline = now + phaseMs(phase);
}

void FocusTimer::skip(unsigned long now)
{
    if (phase == Phase::LONG_BREAK)
    {
        focusesInCycle = 0;
    }
    enter(phase == Phase::FOCUS ? Phase::BREAK : Phase::FOCUS, now, false);
}

void FocusTimer::finish(unsigned long now)
{
    endedPhase = phase;
    phaseEndEvents++;
    Phase next = Phase::FOCUS;
    if (phase == Phase::FOCUS)
    {
        focusesInCycle++;
        next = focusesInCycle >= FOCUSES_PER_CYCLE ? Phase::LONG_BREAK : Phase::BREAK;
    }
    else if (phase == Phase::LONG_BREAK)
    {
        focusesInCycle = 0;
    }
    enter(next, now, autoStart);
}

void FocusTimer::update(unsigned long now)
{
    if (state == TimerState::RUNNING && (long)(now - deadline) >= 0)
    {
        finish(now);
    }
}
