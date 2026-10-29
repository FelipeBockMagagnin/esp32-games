#ifndef FOCUS_TIMER_H
#define FOCUS_TIMER_H

#include <Arduino.h>

enum class Phase : uint8_t
{
    FOCUS,
    BREAK,
    LONG_BREAK
};

enum class TimerState : uint8_t
{
    IDLE, // The phase is set up but waits for Confirm
    RUNNING,
    PAUSED
};

class FocusTimer
{
public:
    static const uint8_t FOCUSES_PER_CYCLE = 4; // The long break comes after this many

    // Minutes per phase, read whenever a phase starts, so a change waits for the next one
    void setDurations(uint8_t focusMin, uint8_t breakMin, uint8_t longBreakMin);
    void setAutoStart(bool on) { autoStart = on; }

    void toggle(unsigned long now);
    void restart();
    // Moves on without counting the phase as done
    void skip(unsigned long now);
    // Ends the phase as if its time ran out
    void finish(unsigned long now);
    void update(unsigned long now);

    Phase getPhase() const { return phase; }
    TimerState getState() const { return state; }
    unsigned long getPhaseMs() const { return phaseMs(phase); }
    unsigned long getRemainingMs(unsigned long now) const;
    uint8_t getFocusesInCycle() const { return focusesInCycle; }
    uint8_t getFocusMinutes() const { return minutes[(uint8_t)Phase::FOCUS]; }

    // Counts every phase that ran out; the phase it was is getEndedPhase()
    uint32_t getPhaseEndEvents() const { return phaseEndEvents; }
    Phase getEndedPhase() const { return endedPhase; }

private:
    Phase phase = Phase::FOCUS;
    TimerState state = TimerState::IDLE;
    uint8_t minutes[3] = {25, 5, 15};
    bool autoStart = false;
    unsigned long deadline = 0;
    unsigned long pausedRemaining = 0;
    uint8_t focusesInCycle = 0;
    uint32_t phaseEndEvents = 0;
    Phase endedPhase = Phase::FOCUS;

    unsigned long phaseMs(Phase p) const { return (unsigned long)minutes[(uint8_t)p] * 60000UL; }
    void enter(Phase next, unsigned long now, bool run);
};

#endif // FOCUS_TIMER_H
