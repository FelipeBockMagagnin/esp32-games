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

    // Low note when the player can't afford the upgrade
    void playError()
    {
        tone(pin, NOTE_A3, 150);
    }
};

#endif // SOUND_MANAGER_H
