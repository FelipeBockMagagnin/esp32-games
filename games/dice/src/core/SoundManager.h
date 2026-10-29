#ifndef SOUND_MANAGER_H
#define SOUND_MANAGER_H

#include <Arduino.h>
#include "pitches.h"

enum class SoundMode : uint8_t
{
    ON,
    QUIET, // Only the results: no clicks, no menu blips
    OFF,
    COUNT
};

// The ESP32 core queues tone() calls, so a run of notes plays in sequence without blocking.
// A click is shorter than the gap between two, or the queue would fall behind the dice.
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

    // A die hitting the table, a semitone either way so a tumble does not drone
    void playClatter()
    {
        if (!allowed(false))
        {
            return;
        }
        float shift = powf(2.0f, (random(5) - 2) / 12.0f);
        tone(pin, (unsigned int)(NOTE_E6 * shift), 6);
    }

    // The wheel's flapper on a peg
    void playClick()
    {
        if (!allowed(false))
        {
            return;
        }
        tone(pin, NOTE_A7, 4);
    }

    void playResult()
    {
        if (!allowed(true))
        {
            return;
        }
        tone(pin, NOTE_G6, 50);
        tone(pin, NOTE_C7, 110);
    }

    void playCritical()
    {
        if (!allowed(true))
        {
            return;
        }
        tone(pin, NOTE_C6, 70);
        tone(pin, NOTE_E6, 70);
        tone(pin, NOTE_G6, 70);
        tone(pin, NOTE_C7, 70);
        tone(pin, NOTE_E7, 70);
        tone(pin, NOTE_G7, 240);
    }

    void playFumble()
    {
        if (!allowed(true))
        {
            return;
        }
        tone(pin, NOTE_D4, 160);
        tone(pin, NOTE_A3, 160);
        tone(pin, NOTE_F3, 400);
    }

    // The hat is empty: a raffle or a bingo is over
    void playEmpty()
    {
        if (!allowed(true))
        {
            return;
        }
        tone(pin, NOTE_G6, 90);
        tone(pin, NOTE_E6, 90);
        tone(pin, NOTE_C6, 200);
    }

    // A change the field refused: already at its limit
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
