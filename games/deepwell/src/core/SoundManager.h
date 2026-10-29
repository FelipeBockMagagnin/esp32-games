#ifndef SOUND_MANAGER_H
#define SOUND_MANAGER_H

#include <Arduino.h>
#include "../pitches.h"

enum class SoundMode : uint8_t
{
    ON,
    QUIET, // Only the moments worth looking up for: level ups, new gear, stairs, death
    OFF,
    COUNT
};

// The ESP32 core queues tone() calls, so a run of notes plays in sequence without blocking
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
        tone(pin, NOTE_GS7, 100);
    }

    // A semitone either way at random, so a run of hits does not drone
    void playHit()
    {
        if (!allowed(false))
        {
            return;
        }
        float shift = powf(2.0f, (random(3) - 1) / 12.0f);
        tone(pin, (unsigned int)(NOTE_G5 * shift), 25);
        tone(pin, (unsigned int)(NOTE_C5 * shift), 45);
    }

    void playKill()
    {
        if (!allowed(false))
        {
            return;
        }
        tone(pin, NOTE_E5, 35);
        tone(pin, NOTE_B6, 110);
    }

    void playHurt()
    {
        if (!allowed(false))
        {
            return;
        }
        tone(pin, NOTE_D4, 50);
        tone(pin, NOTE_A3, 70);
    }

    void playPickup()
    {
        if (!allowed(false))
        {
            return;
        }
        tone(pin, NOTE_C7, 50);
        tone(pin, NOTE_G7, 90);
    }

    void playHeal()
    {
        if (!allowed(false))
        {
            return;
        }
        tone(pin, NOTE_E6, 70);
        tone(pin, NOTE_A6, 70);
        tone(pin, NOTE_E7, 120);
    }

    void playGear()
    {
        if (!allowed(true))
        {
            return;
        }
        tone(pin, NOTE_C6, 70);
        tone(pin, NOTE_G6, 70);
        tone(pin, NOTE_C7, 70);
        tone(pin, NOTE_E7, 70);
        tone(pin, NOTE_G7, 220);
    }

    void playLevelUp()
    {
        if (!allowed(true))
        {
            return;
        }
        tone(pin, NOTE_C6, 80);
        tone(pin, NOTE_E6, 80);
        tone(pin, NOTE_G6, 80);
        tone(pin, NOTE_C7, 160);
    }

    // Falling steps, the way down
    void playStairs()
    {
        if (!allowed(true))
        {
            return;
        }
        tone(pin, NOTE_G6, 90);
        tone(pin, NOTE_E6, 90);
        tone(pin, NOTE_C6, 90);
        tone(pin, NOTE_G5, 200);
    }

    void playDeath()
    {
        if (!allowed(true))
        {
            return;
        }
        tone(pin, NOTE_D4, 160);
        tone(pin, NOTE_A3, 160);
        tone(pin, NOTE_F3, 160);
        tone(pin, NOTE_D3, 500);
    }

    void playStart()
    {
        if (!allowed(false))
        {
            return;
        }
        tone(pin, NOTE_C5, 90);
        tone(pin, NOTE_G5, 90);
        tone(pin, NOTE_C6, 90);
        tone(pin, NOTE_E6, 240);
    }

    // An action that did nothing: a wall, no potions, no stairs
    void playError()
    {
        if (!allowed(false))
        {
            return;
        }
        tone(pin, NOTE_A3, 60);
    }
};

#endif // SOUND_MANAGER_H
