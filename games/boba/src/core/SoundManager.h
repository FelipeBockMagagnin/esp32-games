#ifndef SOUND_MANAGER_H
#define SOUND_MANAGER_H

#include <Arduino.h>
#include "pitches.h"

enum class SoundMode : uint8_t
{
    ON,
    QUIET, // Only the pet calling, growing and dying: no menu blips
    OFF,
    COUNT
};

// The ESP32 core queues tone() calls, so a run of notes plays in sequence without blocking.
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
        if (allowed(false))
        {
            tone(pin, NOTE_GS7, 50);
        }
    }

    void playRefuse()
    {
        if (allowed(false))
        {
            tone(pin, NOTE_C5, 90);
            tone(pin, NOTE_C5, 120);
        }
    }

    void playEat()
    {
        if (allowed(false))
        {
            tone(pin, NOTE_E7, 40);
            tone(pin, NOTE_A7, 40);
        }
    }

    void playCare()
    {
        if (allowed(false))
        {
            tone(pin, NOTE_C7, 60);
            tone(pin, NOTE_G7, 80);
        }
    }

    void playCall()
    {
        if (allowed(true))
        {
            tone(pin, NOTE_E7, 90);
            tone(pin, NOTE_E7, 90);
            tone(pin, NOTE_E7, 90);
        }
    }

    void playHatch()
    {
        if (allowed(true))
        {
            tone(pin, NOTE_C6, 70);
            tone(pin, NOTE_E6, 70);
            tone(pin, NOTE_G6, 70);
            tone(pin, NOTE_C7, 140);
        }
    }

    void playEvolve()
    {
        if (allowed(true))
        {
            tone(pin, NOTE_G6, 80);
            tone(pin, NOTE_C7, 80);
            tone(pin, NOTE_E7, 80);
            tone(pin, NOTE_G7, 200);
        }
    }

    void playDie()
    {
        if (allowed(true))
        {
            tone(pin, NOTE_G5, 180);
            tone(pin, NOTE_DS5, 180);
            tone(pin, NOTE_C5, 180);
            tone(pin, NOTE_G4, 500);
        }
    }

    void playWin()
    {
        if (allowed(false))
        {
            tone(pin, NOTE_C7, 60);
            tone(pin, NOTE_E7, 60);
            tone(pin, NOTE_G7, 100);
        }
    }

    void playLose()
    {
        if (allowed(false))
        {
            tone(pin, NOTE_G5, 80);
            tone(pin, NOTE_E5, 140);
        }
    }
};

#endif // SOUND_MANAGER_H
