#ifndef SOUND_MANAGER_H
#define SOUND_MANAGER_H

#include <Arduino.h>
#include "pitches.h"

enum class SoundMode : uint8_t
{
    ON,
    QUIET, // Only deaths, the boss and the end of a floor
    OFF,
    COUNT
};

// Central place for the buzzer sounds so screens don't need to know the pin or notes.
// The ESP32 core queues tone() calls, so a phrase plays in sequence without blocking.
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

    // Everything heard during play is a few tens of milliseconds: tone() queues its notes, and a
    // long phrase would push the next jump's blip out of step with the jump
    void playJump()
    {
        if (allowed(false))
        {
            tone(pin, NOTE_C6, 25);
            tone(pin, NOTE_G6, 25);
        }
    }

    void playStomp()
    {
        if (allowed(false))
        {
            tone(pin, NOTE_E5, 20);
            tone(pin, NOTE_E6, 40);
        }
    }

    void playDash()
    {
        if (allowed(false))
        {
            tone(pin, NOTE_A6, 15);
            tone(pin, NOTE_A5, 30);
        }
    }

    void playDie()
    {
        if (allowed(true))
        {
            tone(pin, NOTE_B4, 80);
            tone(pin, NOTE_F4, 80);
            tone(pin, NOTE_B3, 180);
        }
    }

    void playShot()
    {
        if (allowed(false))
        {
            tone(pin, NOTE_D3, 20);
            tone(pin, NOTE_A2, 20);
        }
    }

    void playCrack()
    {
        if (allowed(false))
        {
            tone(pin, NOTE_G3, 30);
        }
    }

    void playCheckpoint()
    {
        if (allowed(false))
        {
            tone(pin, NOTE_E6, 50);
            tone(pin, NOTE_A6, 50);
            tone(pin, NOTE_E7, 90);
        }
    }

    void playBossWake()
    {
        if (allowed(true))
        {
            tone(pin, NOTE_C4, 120);
            tone(pin, NOTE_DS4, 120);
            tone(pin, NOTE_FS4, 120);
            tone(pin, NOTE_A3, 240);
        }
    }

    void playBossCharge()
    {
        if (allowed(false))
        {
            tone(pin, NOTE_A3, 40);
            tone(pin, NOTE_A4, 40);
        }
    }

    void playBossThud()
    {
        if (allowed(false))
        {
            tone(pin, NOTE_C3, 60);
        }
    }

    void playBossHit()
    {
        if (allowed(true))
        {
            tone(pin, NOTE_G5, 40);
            tone(pin, NOTE_C6, 40);
            tone(pin, NOTE_G6, 80);
        }
    }

    void playBossDown()
    {
        if (allowed(true))
        {
            tone(pin, NOTE_C6, 80);
            tone(pin, NOTE_G5, 80);
            tone(pin, NOTE_E5, 80);
            tone(pin, NOTE_C5, 80);
            tone(pin, NOTE_C6, 240);
        }
    }

    // One syllable of the coworker's speech, pitched per line
    void playTalk(uint8_t line)
    {
        if (allowed(false))
        {
            tone(pin, (line % 2) ? NOTE_D6 : NOTE_F6, 30);
        }
    }

    // The floor is cleared and there is still no coffee: a cheerful start that sags
    void playFloorDone()
    {
        if (allowed(true))
        {
            tone(pin, NOTE_C6, 100);
            tone(pin, NOTE_E6, 100);
            tone(pin, NOTE_G6, 100);
            tone(pin, NOTE_FS6, 160);
            tone(pin, NOTE_F6, 160);
            tone(pin, NOTE_E6, 360);
        }
    }

    // Boot jingle under the splash logo, about 1.2 s, ending inside its time on screen
    void playJingle()
    {
        if (!allowed(false))
        {
            return;
        }
        tone(pin, NOTE_C5, 90);
        tone(pin, NOTE_G5, 90);
        tone(pin, NOTE_C6, 90);
        tone(pin, NOTE_G5, 90);
        tone(pin, NOTE_E6, 180);
        tone(pin, NOTE_D6, 90);
        tone(pin, NOTE_E6, 90);
        tone(pin, NOTE_G6, 380);
    }

    void playError()
    {
        if (allowed(false))
        {
            tone(pin, NOTE_A3, 150);
        }
    }
};

#endif // SOUND_MANAGER_H
