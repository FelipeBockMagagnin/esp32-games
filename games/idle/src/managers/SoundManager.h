#ifndef SOUND_MANAGER_H
#define SOUND_MANAGER_H

#include <Arduino.h>
#include "../pitches.h"

// Central place for the buzzer sounds so screens don't need to know the pin or notes
class SoundManager
{
private:
    int pin;

public:
    SoundManager(int pin) : pin(pin) {}

    void playMenu()
    {
        tone(pin, NOTE_GS7, 100);
    }

    // Kept short so fast repeated clicks don't blur together
    void playMine()
    {
        tone(pin, NOTE_E6, 30);
    }

    // Rising arpeggio; the ESP32 core queues tone() calls, so the notes play in sequence without blocking
    void playLevelUp()
    {
        tone(pin, NOTE_C6, 80);
        tone(pin, NOTE_E6, 80);
        tone(pin, NOTE_G6, 80);
        tone(pin, NOTE_C7, 160);
    }

    // High note for a successful purchase
    void playBuy()
    {
        tone(pin, NOTE_C7, 120);
    }

    // Longer fanfare than a level-up, so an achievement is unmistakable
    void playAchievement()
    {
        tone(pin, NOTE_E6, 90);
        tone(pin, NOTE_G6, 90);
        tone(pin, NOTE_C7, 90);
        tone(pin, NOTE_E7, 90);
        tone(pin, NOTE_G7, 220);
    }

    // Two-step call when a new screen opens, distinct from the achievement fanfare
    void playUnlock()
    {
        tone(pin, NOTE_G6, 120);
        tone(pin, NOTE_C7, 120);
        tone(pin, NOTE_G6, 120);
        tone(pin, NOTE_C7, 260);
    }

    // Bright rising run for a new item, a step above the level-up so a drop stands out
    void playDrop()
    {
        tone(pin, NOTE_C6, 70);
        tone(pin, NOTE_G6, 70);
        tone(pin, NOTE_C7, 70);
        tone(pin, NOTE_E7, 70);
        tone(pin, NOTE_G7, 260);
    }

    // Short falling thud when an enemy's hit lands, so a hit is heard from any screen glance
    void playHurt()
    {
        tone(pin, NOTE_D4, 50);
        tone(pin, NOTE_A3, 70);
    }

    // Low note when the player can't afford the upgrade
    void playError()
    {
        tone(pin, NOTE_A3, 150);
    }
};

#endif // SOUND_MANAGER_H
