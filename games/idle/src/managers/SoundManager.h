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
        // A semitone either way at random, so a run of strikes does not drone
        float shift = powf(2.0f, (random(3) - 1) / 12.0f);
        tone(pin, (unsigned int)(NOTE_G5 * shift), 25);
        tone(pin, (unsigned int)(NOTE_C5 * shift), 45);
    }

    // A thud and a ring for a kill without a drop, shorter than the level-up arpeggio
    void playKill()
    {
        if (!allowed(false))
        {
            return;
        }
        tone(pin, NOTE_E5, 35);
        tone(pin, NOTE_B6, 110);
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

    // Two quick rising notes, a till's "ka-ching", for a successful purchase. Purchases in
    // quick succession climb a semitone each, up to an octave, so a buying spree is heard.
    // A quick one is a single blip, short enough for a held Confirm's fastest repeat, which
    // would otherwise queue notes that play on after the button is let go.
    void playBuy(bool quick = false)
    {
        unsigned long now = millis();
        buyStreak = now - lastBuyAt < BUY_STREAK_MS ? (buyStreak < 12 ? buyStreak + 1 : 12) : 0;
        lastBuyAt = now;
        if (!allowed(false))
        {
            return;
        }
        float shift = powf(2.0f, buyStreak / 12.0f);
        if (quick)
        {
            tone(pin, (unsigned int)(NOTE_G7 * shift), 30);
            return;
        }
        tone(pin, (unsigned int)(NOTE_C7 * shift), 50);
        tone(pin, (unsigned int)(NOTE_G7 * shift), 110);
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

    // A slow climb through two octaves and a held top note, about as long as the
    // ascension's points take to count up
    void playAscend()
    {
        if (!allowed(true))
        {
            return;
        }
        tone(pin, NOTE_C5, 140);
        tone(pin, NOTE_G5, 140);
        tone(pin, NOTE_C6, 140);
        tone(pin, NOTE_E6, 140);
        tone(pin, NOTE_G6, 140);
        tone(pin, NOTE_C7, 140);
        tone(pin, NOTE_E7, 140);
        tone(pin, NOTE_G7, 140);
        tone(pin, NOTE_C8, 500);
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
