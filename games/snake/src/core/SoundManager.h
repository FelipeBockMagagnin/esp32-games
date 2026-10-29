#ifndef SOUND_MANAGER_H
#define SOUND_MANAGER_H

#include <Arduino.h>
#include "pitches.h"

enum class SoundMode : uint8_t
{
    ON,
    QUIET, // Only eating, dying and records: no menu blips
    OFF,
    COUNT
};

// The ESP32 core queues tone() calls, so a run of notes plays in sequence without blocking.
// Every note during a run is shorter than the fastest step, or the queue would fall behind.
class SoundManager
{
private:
    int pin;
    SoundMode mode = SoundMode::ON;

    bool allowed(bool important) const
    {
        return mode == SoundMode::ON || (important && mode == SoundMode::QUIET);
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
        tone(pin, NOTE_GS7, 60);
    }

    void playStart()
    {
        if (!allowed(false))
        {
            return;
        }
        tone(pin, NOTE_C6, 40);
        tone(pin, NOTE_G6, 60);
    }

    void playEat()
    {
        if (!allowed(true))
        {
            return;
        }
        tone(pin, NOTE_E7, 25);
        tone(pin, NOTE_A7, 25);
    }

    void playPause()
    {
        if (!allowed(false))
        {
            return;
        }
        tone(pin, NOTE_A6, 40);
        tone(pin, NOTE_E6, 60);
    }

    void playDie()
    {
        if (!allowed(true))
        {
            return;
        }
        tone(pin, NOTE_G5, 90);
        tone(pin, NOTE_DS5, 90);
        tone(pin, NOTE_C5, 90);
        tone(pin, NOTE_G4, 300);
    }

    void playRecord()
    {
        if (!allowed(true))
        {
            return;
        }
        tone(pin, NOTE_C6, 80);
        tone(pin, NOTE_E6, 80);
        tone(pin, NOTE_G6, 80);
        tone(pin, NOTE_C7, 240);
    }
};

#endif // SOUND_MANAGER_H
