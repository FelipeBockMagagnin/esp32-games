#ifndef SOUND_MANAGER_H
#define SOUND_MANAGER_H

#include <Arduino.h>
#include "../pitches.h"

enum class SoundMode : uint8_t
{
    ON,
    QUIET, // Only what is worth looking up for: awards, new screens, gold veins, new items
    OFF,
    COUNT
};

// Central place for the buzzer sounds so screens don't need to know the pin or notes
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

    // Kept short so fast repeated clicks don't blur together. A running combo raises it a
    // semitone per step, capped at an octave, so a streak is heard climbing.
    void playMine(uint8_t semitones = 0)
    {
        if (!allowed(false))
        {
            return;
        }
        uint8_t steps = semitones < 12 ? semitones : 12;
        tone(pin, (unsigned int)(NOTE_E6 * powf(2.0f, steps / 12.0f)), 30);
    }

    // Lower and punchier than a mining click, so the strike lands with some weight
    void playStrike()
    {
        if (!allowed(false))
        {
            return;
        }
        tone(pin, NOTE_G5, 25);
        tone(pin, NOTE_C5, 45);
    }

    // Rising arpeggio; the ESP32 core queues tone() calls, so the notes play in sequence without blocking
    void playLevelUp()
    {
        if (!allowed(false))
        {
            return;
        }
        tone(pin, NOTE_C6, 80);
        tone(pin, NOTE_E6, 80);
        tone(pin, NOTE_G6, 80);
        tone(pin, NOTE_C7, 160);
    }

    // Two quick rising notes, a till's "ka-ching", for a successful purchase
    void playBuy()
    {
        if (!allowed(false))
        {
            return;
        }
        tone(pin, NOTE_C7, 50);
        tone(pin, NOTE_G7, 110);
    }

    // Longer fanfare than a level-up, so an achievement is unmistakable
    void playAchievement()
    {
        if (!allowed(true))
        {
            return;
        }
        tone(pin, NOTE_E6, 90);
        tone(pin, NOTE_G6, 90);
        tone(pin, NOTE_C7, 90);
        tone(pin, NOTE_E7, 90);
        tone(pin, NOTE_G7, 220);
    }

    // Two-step call when a new screen opens, distinct from the achievement fanfare
    void playUnlock()
    {
        if (!allowed(true))
        {
            return;
        }
        tone(pin, NOTE_G6, 120);
        tone(pin, NOTE_C7, 120);
        tone(pin, NOTE_G6, 120);
        tone(pin, NOTE_C7, 260);
    }

    // Bright rising run for a new item, a step above the level-up so a drop stands out
    void playDrop()
    {
        if (!allowed(true))
        {
            return;
        }
        tone(pin, NOTE_C6, 70);
        tone(pin, NOTE_G6, 70);
        tone(pin, NOTE_C7, 70);
        tone(pin, NOTE_E7, 70);
        tone(pin, NOTE_G7, 260);
    }

    // Short falling thud when an enemy's hit lands, so a hit is heard from any screen glance
    void playHurt()
    {
        if (!allowed(false))
        {
            return;
        }
        tone(pin, NOTE_D4, 50);
        tone(pin, NOTE_A3, 70);
    }

    // Low note when the player can't afford the upgrade
    void playError()
    {
        if (!allowed(false))
        {
            return;
        }
        tone(pin, NOTE_A3, 150);
    }
};

#endif // SOUND_MANAGER_H
