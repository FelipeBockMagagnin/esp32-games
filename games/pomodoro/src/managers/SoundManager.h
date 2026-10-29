#ifndef SOUND_MANAGER_H
#define SOUND_MANAGER_H

#include <Arduino.h>
#include "pitches.h"

enum class SoundMode : uint8_t
{
    ON,
    QUIET, // Only the alarms
    OFF,
    COUNT
};

// The ESP32 core queues tone() calls, so a melody plays its notes in sequence without blocking
class SoundManager
{
private:
    static const unsigned long ALARM_REPEAT_MS = 2500;
    static const unsigned long ALARM_GIVE_UP_MS = 60000;

    int pin;
    SoundMode mode = SoundMode::ON;
    bool ringing = false;
    bool ringingFocus = false;
    unsigned long nextRingAt = 0;
    unsigned long ringUntil = 0;

    bool allowed(bool important) const
    {
        return mode == SoundMode::ON || (important && mode == SoundMode::QUIET);
    }

    // Falling after a focus (time to rest), rising after a break (back to work)
    void playAlarmOnce()
    {
        if (ringingFocus)
        {
            tone(pin, NOTE_C7, 120);
            tone(pin, NOTE_G6, 120);
            tone(pin, NOTE_E6, 120);
            tone(pin, NOTE_C6, 300);
        }
        else
        {
            tone(pin, NOTE_C6, 120);
            tone(pin, NOTE_E6, 120);
            tone(pin, NOTE_G6, 120);
            tone(pin, NOTE_C7, 300);
        }
    }

public:
    SoundManager(int pin) : pin(pin) {}

    void setMode(SoundMode m) { mode = m; }
    SoundMode getMode() const { return mode; }

    void playMenu()
    {
        if (!allowed(false))
        {
            return;
        }
        tone(pin, NOTE_GS7, 100);
    }

    void playStart()
    {
        if (!allowed(false))
        {
            return;
        }
        tone(pin, NOTE_C7, 50);
        tone(pin, NOTE_G7, 110);
    }

    void playPause()
    {
        if (!allowed(false))
        {
            return;
        }
        tone(pin, NOTE_G6, 50);
        tone(pin, NOTE_C6, 110);
    }

    // Repeats until stopAlarm(), or gives up after a minute so an empty room is not rung forever
    void startAlarm(bool focusEnded, unsigned long now)
    {
        if (!allowed(true))
        {
            return;
        }
        ringing = true;
        ringingFocus = focusEnded;
        nextRingAt = now;
        ringUntil = now + ALARM_GIVE_UP_MS;
    }

    void stopAlarm() { ringing = false; }
    bool isRinging() const { return ringing; }

    void update(unsigned long now)
    {
        if (!ringing)
        {
            return;
        }
        if ((long)(now - ringUntil) >= 0)
        {
            ringing = false;
            return;
        }
        if ((long)(now - nextRingAt) >= 0)
        {
            playAlarmOnce();
            nextRingAt = now + ALARM_REPEAT_MS;
        }
    }
};

#endif // SOUND_MANAGER_H
