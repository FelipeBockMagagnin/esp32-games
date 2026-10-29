#ifndef SOUND_MANAGER_H
#define SOUND_MANAGER_H

#include <Arduino.h>
#include "pitches.h"

enum class SoundMode : uint8_t
{
    ON,
    QUIET, // Only its calls, hatching and growing up
    OFF,
    COUNT
};

// The ESP32 core queues tone() calls, so a run of notes plays in sequence without blocking.
// Every sound is short and soft-pitched: it is a pet on a desk, never an alarm.
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
            tone(pin, NOTE_GS7, 60);
        }
    }

    // Refused: it is full, asleep, nothing to sweep
    void playError()
    {
        if (allowed(false))
        {
            tone(pin, NOTE_E4, 50);
            tone(pin, NOTE_C4, 80);
        }
    }

    // A form's chirp: its note, then a fifth and back
    void playVoice(uint16_t hz)
    {
        if (allowed(false) && hz > 0)
        {
            tone(pin, hz, 60);
            tone(pin, hz * 3 / 2, 60);
            tone(pin, hz, 90);
        }
    }

    void playEat()
    {
        if (allowed(false))
        {
            tone(pin, NOTE_G5, 40);
            tone(pin, NOTE_D5, 40);
            tone(pin, NOTE_G5, 40);
            tone(pin, NOTE_D5, 40);
        }
    }

    void playClean()
    {
        if (allowed(false))
        {
            tone(pin, NOTE_C7, 40);
            tone(pin, NOTE_E7, 40);
            tone(pin, NOTE_G7, 80);
        }
    }

    void playMeds()
    {
        if (allowed(false))
        {
            tone(pin, NOTE_E6, 70);
            tone(pin, NOTE_A6, 70);
            tone(pin, NOTE_E7, 120);
        }
    }

    // A need ran low: two soft notes, rate-limited by the main loop
    void playCall(uint16_t hz)
    {
        if (allowed(true) && hz > 0)
        {
            tone(pin, hz, 90);
            tone(pin, hz * 4 / 5, 140);
        }
    }

    // Falling, slow: lights out
    void playSleep()
    {
        if (allowed(false))
        {
            tone(pin, NOTE_G5, 120);
            tone(pin, NOTE_E5, 120);
            tone(pin, NOTE_C5, 240);
        }
    }

    void playLamp(bool on)
    {
        if (allowed(false))
        {
            tone(pin, on ? NOTE_A6 : NOTE_A5, 40);
        }
    }

    void playHatch()
    {
        if (allowed(true))
        {
            tone(pin, NOTE_C6, 80);
            tone(pin, NOTE_E6, 80);
            tone(pin, NOTE_G6, 80);
            tone(pin, NOTE_C7, 80);
            tone(pin, NOTE_G6, 80);
            tone(pin, NOTE_C7, 240);
        }
    }

    void playEvolve()
    {
        if (allowed(true))
        {
            tone(pin, NOTE_C6, 90);
            tone(pin, NOTE_G6, 90);
            tone(pin, NOTE_E6, 90);
            tone(pin, NOTE_C7, 90);
            tone(pin, NOTE_G7, 300);
        }
    }

    // The Hum minigame's notes
    void playNote(uint16_t hz, uint16_t ms)
    {
        if (mode != SoundMode::OFF)
        {
            tone(pin, hz, ms);
        }
    }

    void playWin()
    {
        if (allowed(false))
        {
            tone(pin, NOTE_E6, 80);
            tone(pin, NOTE_G6, 80);
            tone(pin, NOTE_C7, 200);
        }
    }

    void playMiss()
    {
        if (allowed(false))
        {
            tone(pin, NOTE_C5, 90);
            tone(pin, NOTE_B4, 90);
            tone(pin, NOTE_AS4, 200);
        }
    }

    void playStart()
    {
        if (allowed(false))
        {
            tone(pin, NOTE_E6, 70);
            tone(pin, NOTE_G6, 70);
            tone(pin, NOTE_E7, 140);
        }
    }
};

#endif // SOUND_MANAGER_H
