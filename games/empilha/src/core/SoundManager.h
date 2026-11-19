#ifndef SOUND_MANAGER_H
#define SOUND_MANAGER_H

#include <Arduino.h>
#include "pitches.h"
#include "Volume.h"

enum class SoundMode : uint8_t
{
    ON,
    QUIET, // Only clears, levels and the end of a run: no menu blips, steps or landings
    OFF,
    COUNT
};

// playTone() queues its notes, so a run of notes plays in sequence without blocking.
// A note played on every move is shorter than the key repeat, or the queue would fall behind
// the piece.
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
            playTone(pin, NOTE_GS7, 60);
        }
    }

    void playMove()
    {
        if (allowed(false))
        {
            playTone(pin, NOTE_E7, 5);
        }
    }

    void playTurn()
    {
        if (allowed(false))
        {
            playTone(pin, NOTE_A6, 8);
            playTone(pin, NOTE_E7, 8);
        }
    }

    void playLand()
    {
        if (allowed(false))
        {
            playTone(pin, NOTE_E4, 16);
        }
    }

    void playSlam()
    {
        if (allowed(false))
        {
            playTone(pin, NOTE_C5, 8);
            playTone(pin, NOTE_E4, 8);
            playTone(pin, NOTE_B3, 22);
        }
    }

    void playPause()
    {
        if (allowed(false))
        {
            playTone(pin, NOTE_A6, 40);
            playTone(pin, NOTE_E6, 60);
        }
    }

    // More rows, a longer climb
    void playRows(uint8_t count)
    {
        static const uint16_t NOTES[] = {NOTE_D6, NOTE_F6, NOTE_A6, NOTE_D7, NOTE_F7};
        static const uint8_t NOTE_COUNT = sizeof(NOTES) / sizeof(NOTES[0]);
        if (!allowed(true))
        {
            return;
        }
        for (uint8_t i = 0; i < count + 1 && i < NOTE_COUNT; i++)
        {
            playTone(pin, NOTES[i], 34);
        }
    }

    // Each link of a cascade rings a step higher
    void playChain(uint8_t link)
    {
        static const uint16_t NOTES[] = {NOTE_E6, NOTE_G6, NOTE_B6, NOTE_E7, NOTE_G7, NOTE_B7};
        static const uint8_t NOTE_COUNT = sizeof(NOTES) / sizeof(NOTES[0]);
        if (!allowed(true))
        {
            return;
        }
        uint8_t top = link < NOTE_COUNT ? link : NOTE_COUNT - 1;
        playTone(pin, NOTES[top], 40);
        playTone(pin, NOTES[top] * 2, 60);
    }

    void playLevel()
    {
        if (!allowed(true))
        {
            return;
        }
        playTone(pin, NOTE_G6, 50);
        playTone(pin, NOTE_D7, 50);
        playTone(pin, NOTE_G7, 90);
    }

    void playTopOut()
    {
        if (!allowed(true))
        {
            return;
        }
        playTone(pin, NOTE_A5, 60);
        playTone(pin, NOTE_F5, 60);
        playTone(pin, NOTE_D5, 60);
        playTone(pin, NOTE_A4, 80);
        playTone(pin, NOTE_F4, 80);
        playTone(pin, NOTE_D4, 200);
    }

    void playFinished()
    {
        if (!allowed(true))
        {
            return;
        }
        playTone(pin, NOTE_D6, 70);
        playTone(pin, NOTE_A6, 70);
        playTone(pin, NOTE_D7, 70);
        playTone(pin, NOTE_A6, 70);
        playTone(pin, NOTE_D7, 200);
    }

    void playRecord()
    {
        if (!allowed(true))
        {
            return;
        }
        playTone(pin, NOTE_A6, 80);
        playTone(pin, NOTE_D7, 80);
        playTone(pin, NOTE_FS7, 80);
        playTone(pin, NOTE_A7, 240);
    }
};

#endif // SOUND_MANAGER_H
