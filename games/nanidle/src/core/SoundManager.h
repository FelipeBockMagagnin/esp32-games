#ifndef SOUND_MANAGER_H
#define SOUND_MANAGER_H

#include <Arduino.h>
#include "pitches.h"

enum class SoundMode : uint8_t
{
    ON,
    QUIET, // Only bosses, deaths, unlocks and rebirths: no clicks or hits
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
            tone(pin, NOTE_GS7, 20);
        }
    }

    void playDenied()
    {
        if (allowed(false))
        {
            tone(pin, NOTE_E5, 40);
            tone(pin, NOTE_C5, 70);
        }
    }

    void playBuy()
    {
        if (allowed(false))
        {
            tone(pin, NOTE_C7, 25);
            tone(pin, NOTE_G7, 35);
        }
    }

    void playHit(bool strike)
    {
        if (allowed(false))
        {
            tone(pin, strike ? NOTE_A5 : NOTE_E6, strike ? 30 : 10);
        }
    }

    void playKill()
    {
        if (allowed(false))
        {
            tone(pin, NOTE_E6, 30);
            tone(pin, NOTE_B6, 50);
        }
    }

    void playDrop()
    {
        if (allowed(false))
        {
            tone(pin, NOTE_G6, 40);
            tone(pin, NOTE_D7, 40);
            tone(pin, NOTE_G7, 70);
        }
    }

    void playBoss()
    {
        if (!allowed(true))
        {
            return;
        }
        tone(pin, NOTE_C6, 70);
        tone(pin, NOTE_E6, 70);
        tone(pin, NOTE_G6, 70);
        tone(pin, NOTE_C7, 160);
    }

    void playDeath()
    {
        if (!allowed(true))
        {
            return;
        }
        tone(pin, NOTE_G5, 80);
        tone(pin, NOTE_DS5, 80);
        tone(pin, NOTE_C5, 200);
    }

    // A message box popping up
    void playNotice()
    {
        if (!allowed(true))
        {
            return;
        }
        tone(pin, NOTE_A6, 60);
        tone(pin, NOTE_E7, 120);
    }

    void playStartup()
    {
        if (!allowed(true))
        {
            return;
        }
        tone(pin, NOTE_G5, 120);
        tone(pin, NOTE_D6, 120);
        tone(pin, NOTE_G6, 120);
        tone(pin, NOTE_B6, 120);
        tone(pin, NOTE_D7, 300);
    }

    void playRebirth()
    {
        if (!allowed(true))
        {
            return;
        }
        for (uint16_t note : {NOTE_C5, NOTE_G5, NOTE_C6, NOTE_E6, NOTE_G6, NOTE_C7})
        {
            tone(pin, note, 70);
        }
        tone(pin, NOTE_C8, 250);
    }
};

#endif // SOUND_MANAGER_H
