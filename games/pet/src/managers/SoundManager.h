#ifndef SOUND_MANAGER_H
#define SOUND_MANAGER_H

#include <Arduino.h>
#include "pitches.h"
#include "Volume.h"

enum class SoundMode : uint8_t
{
    ON,
    QUIET, // Only what is worth looking up for: births, departures, new species
    OFF,
    COUNT
};

// Central place for the buzzer sounds so screens don't need to know the pin or notes.
// playTone() queues its notes, so a phrase plays in sequence without blocking.
class SoundManager
{
private:
    static const unsigned long BUY_STREAK_MS = 1000;

    int pin;
    SoundMode mode = SoundMode::ON;
    unsigned long lastBuyAt = 0;
    uint8_t buyStreak = 0;

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
        playTone(pin, NOTE_GS7, 100);
    }

    // Short enough that a held arrow sweeping the cursor does not blur into a drone
    void playStep()
    {
        if (!allowed(false))
        {
            return;
        }
        playTone(pin, NOTE_E7, 15);
    }

    void playWater()
    {
        if (!allowed(false))
        {
            return;
        }
        playTone(pin, NOTE_C6, 30);
        playTone(pin, NOTE_G5, 60);
    }

    void playHarvest()
    {
        if (!allowed(false))
        {
            return;
        }
        playTone(pin, NOTE_E6, 35);
        playTone(pin, NOTE_B6, 110);
    }

    // Purchases in quick succession climb a semitone each, up to an octave
    void playBuy()
    {
        unsigned long now = millis();
        buyStreak = now - lastBuyAt < BUY_STREAK_MS ? (buyStreak < 12 ? buyStreak + 1 : 12) : 0;
        lastBuyAt = now;
        if (!allowed(false))
        {
            return;
        }
        float shift = powf(2.0f, buyStreak / 12.0f);
        playTone(pin, (unsigned int)(NOTE_C7 * shift), 50);
        playTone(pin, (unsigned int)(NOTE_G7 * shift), 110);
    }

    void playBirth()
    {
        if (!allowed(true))
        {
            return;
        }
        playTone(pin, NOTE_C6, 80);
        playTone(pin, NOTE_E6, 80);
        playTone(pin, NOTE_G6, 80);
        playTone(pin, NOTE_C7, 160);
    }

    // The one phrase that falls, so a critter walking out is heard from across the room
    void playLeave()
    {
        if (!allowed(true))
        {
            return;
        }
        playTone(pin, NOTE_D5, 120);
        playTone(pin, NOTE_A4, 120);
        playTone(pin, NOTE_F4, 300);
    }

    // Boot jingle under the splash logo, about 1.2 s, ending inside its time on screen
    void playJingle()
    {
        if (!allowed(false))
        {
            return;
        }
        playTone(pin, NOTE_C5, 90);
        playTone(pin, NOTE_G5, 90);
        playTone(pin, NOTE_C6, 90);
        playTone(pin, NOTE_G5, 90);
        playTone(pin, NOTE_E6, 180);
        playTone(pin, NOTE_D6, 90);
        playTone(pin, NOTE_E6, 90);
        playTone(pin, NOTE_G6, 380);
    }

    void playError()
    {
        if (!allowed(false))
        {
            return;
        }
        playTone(pin, NOTE_A3, 150);
    }
};

#endif // SOUND_MANAGER_H
