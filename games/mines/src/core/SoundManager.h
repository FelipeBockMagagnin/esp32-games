#ifndef SOUND_MANAGER_H
#define SOUND_MANAGER_H

#include <Arduino.h>
#include "pitches.h"

enum class SoundMode : uint8_t
{
    ON,
    QUIET, // Only flags, blasts, wins and records: no menu blips, steps or digging
    OFF,
    COUNT
};

// The ESP32 core queues tone() calls, so a run of notes plays in sequence without blocking.
// A note played once per animation frame is shorter than the frame, or the queue would fall
// behind the picture.
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

    void playStep()
    {
        if (allowed(false))
        {
            tone(pin, NOTE_E7, 6);
        }
    }

    void playDenied()
    {
        if (allowed(false))
        {
            tone(pin, NOTE_C5, 30);
            tone(pin, NOTE_A4, 40);
        }
    }

    // A lone tile: the higher its number, the higher the note
    void playOpen(uint8_t count)
    {
        static const uint16_t NOTES[9] = {NOTE_G6, NOTE_A5, NOTE_C6, NOTE_D6, NOTE_E6,
                                          NOTE_G6, NOTE_A6, NOTE_C7, NOTE_D7};
        if (allowed(false))
        {
            tone(pin, NOTES[count < 9 ? count : 8], 25);
        }
    }

    // One note per ring of a flood, climbing a pentatonic scale
    void playWave(uint16_t wave)
    {
        static const uint16_t NOTES[] = {NOTE_C6, NOTE_D6, NOTE_E6, NOTE_G6, NOTE_A6, NOTE_C7,
                                         NOTE_D7, NOTE_E7, NOTE_G7, NOTE_A7, NOTE_C8};
        static const uint8_t COUNT = sizeof(NOTES) / sizeof(NOTES[0]);
        if (allowed(false))
        {
            tone(pin, NOTES[wave < COUNT ? wave : COUNT - 1], 18);
        }
    }

    void playChord()
    {
        if (allowed(false))
        {
            tone(pin, NOTE_E6, 15);
            tone(pin, NOTE_A6, 15);
        }
    }

    void playFlag(bool planted)
    {
        if (!allowed(true))
        {
            return;
        }
        tone(pin, planted ? NOTE_E6 : NOTE_B6, 30);
        tone(pin, planted ? NOTE_B6 : NOTE_E6, 40);
    }

    void playPause()
    {
        if (allowed(false))
        {
            tone(pin, NOTE_A6, 40);
            tone(pin, NOTE_E6, 60);
        }
    }

    void playBlast()
    {
        if (!allowed(true))
        {
            return;
        }
        tone(pin, NOTE_C5, 30);
        tone(pin, NOTE_A4, 30);
        tone(pin, NOTE_F4, 30);
        tone(pin, NOTE_D4, 40);
        tone(pin, NOTE_B3, 60);
        tone(pin, NOTE_G3, 160);
    }

    // Each mine that turns up after the blast, a low knock
    void playChain(uint8_t i)
    {
        if (allowed(true))
        {
            tone(pin, (i & 1) ? NOTE_D4 : NOTE_F4, 14);
        }
    }

    void playWin()
    {
        if (!allowed(true))
        {
            return;
        }
        tone(pin, NOTE_C6, 70);
        tone(pin, NOTE_E6, 70);
        tone(pin, NOTE_G6, 70);
        tone(pin, NOTE_C7, 70);
        tone(pin, NOTE_G6, 70);
        tone(pin, NOTE_C7, 200);
    }

    void playRecord()
    {
        if (!allowed(true))
        {
            return;
        }
        tone(pin, NOTE_G6, 80);
        tone(pin, NOTE_C7, 80);
        tone(pin, NOTE_E7, 80);
        tone(pin, NOTE_G7, 240);
    }
};

#endif // SOUND_MANAGER_H
