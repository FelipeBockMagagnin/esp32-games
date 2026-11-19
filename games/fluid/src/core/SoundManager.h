#ifndef SOUND_MANAGER_H
#define SOUND_MANAGER_H

#include <Arduino.h>
#include "pitches.h"
#include "Volume.h"

enum class SoundMode : uint8_t
{
    ON,
    QUIET, // Only the field's sounds: no menu blips
    OFF,
    COUNT
};

// playTone() queues its notes, so a run of notes plays in sequence without blocking
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

    void playStart()
    {
        if (allowed(false))
        {
            playTone(pin, NOTE_C6, 40);
            playTone(pin, NOTE_G6, 60);
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

    // A drop: a quick slide down, pitched by the direction the water now falls
    void playTilt(uint8_t dir)
    {
        if (!allowed(true))
        {
            return;
        }
        static const uint16_t FROM[4] = {NOTE_E6, NOTE_C5, NOTE_G5, NOTE_A5};
        playTone(pin, FROM[dir & 3], 18);
        playTone(pin, FROM[dir & 3] * 3 / 4, 18);
        playTone(pin, FROM[dir & 3] / 2, 30);
    }

    void playShake()
    {
        if (!allowed(true))
        {
            return;
        }
        playTone(pin, NOTE_C5, 25);
        playTone(pin, NOTE_G5, 25);
        playTone(pin, NOTE_C6, 25);
        playTone(pin, NOTE_E5, 40);
    }

    void playClear()
    {
        if (!allowed(true))
        {
            return;
        }
        playTone(pin, NOTE_G6, 40);
        playTone(pin, NOTE_D6, 40);
        playTone(pin, NOTE_G5, 40);
        playTone(pin, NOTE_D5, 80);
    }
};

#endif // SOUND_MANAGER_H
