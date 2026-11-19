#ifndef SOUND_MANAGER_H
#define SOUND_MANAGER_H

#include <Arduino.h>
#include "pitches.h"
#include "Volume.h"

enum class SoundMode : uint8_t
{
    ON,
    GAMES, // Only the games' music: no menu blips
    OFF,
    COUNT
};

// The menus' blips, and the one note a game plays at a time (Apu picks it). playTone()
// queues its notes, so neither blocks.
class SoundManager
{
private:
    int pin;
    SoundMode mode = SoundMode::ON;
    uint16_t note = 0;

public:
    SoundManager(int pin) : pin(pin) {}

    void setMode(SoundMode m) { mode = m; }
    SoundMode getMode() const { return mode; }

    void playMenu()
    {
        if (mode == SoundMode::ON)
        {
            playTone(pin, NOTE_GS7, 60);
        }
    }

    void playStart()
    {
        if (mode == SoundMode::ON)
        {
            playTone(pin, NOTE_C6, 40);
            playTone(pin, NOTE_G6, 60);
        }
    }

    // Called every frame of a game; only a change of note reaches the buzzer
    void playNote(uint16_t hz)
    {
        if (mode == SoundMode::OFF)
        {
            hz = 0;
        }
        if (hz == note)
        {
            return;
        }
        note = hz;
        if (hz == 0)
        {
            stopTone(pin);
        }
        else
        {
            playTone(pin, hz);
        }
    }
};

#endif // SOUND_MANAGER_H
