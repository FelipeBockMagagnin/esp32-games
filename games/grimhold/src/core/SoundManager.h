#ifndef SOUND_MANAGER_H
#define SOUND_MANAGER_H

#include <Arduino.h>
#include "pitches.h"

enum class SoundMode : uint8_t
{
    ON,
    QUIET, // Only pickups, the boss, the portal and death: no blows or menu blips
    OFF,
    COUNT
};

// The ESP32 core queues tone() calls, so a phrase plays in sequence without blocking. Everything
// heard in a fight is a few tens of milliseconds, or the queue would trail behind the blows.
class SoundManager
{
private:
    int pin;
    SoundMode mode = SoundMode::ON;

    bool allowed(bool important) const
    {
        return mode == SoundMode::ON || (important && mode == SoundMode::QUIET);
    }

    void play(bool important, std::initializer_list<uint32_t> notes)
    {
        if (!allowed(important))
        {
            return;
        }
        // Pairs of frequency and duration
        const uint32_t *n = notes.begin();
        for (size_t i = 0; i + 1 < notes.size(); i += 2)
        {
            tone(pin, n[i], n[i + 1]);
        }
    }

public:
    SoundManager(int pin) : pin(pin) {}

    void setMode(SoundMode m) { mode = m; }
    SoundMode getMode() const { return mode; }

    void playMenu() { play(false, {NOTE_GS7, 50}); }
    void playError() { play(false, {NOTE_A3, 90}); }
    void playSwing() { play(false, {NOTE_C4, 15, NOTE_G3, 20}); }
    void playHit() { play(false, {NOTE_E3, 25, NOTE_C3, 25}); }
    void playKill() { play(false, {NOTE_G4, 30, NOTE_D4, 30, NOTE_G3, 50}); }
    void playCast() { play(false, {NOTE_A5, 20, NOTE_E6, 20, NOTE_A6, 30}); }
    void playCharged() { play(false, {NOTE_E7, 25}); }
    void playBlast() { play(false, {NOTE_D3, 30, NOTE_A2, 40, NOTE_D2, 60}); }
    void playHurt() { play(false, {NOTE_DS3, 40, NOTE_C3, 40}); }
    void playAlert() { play(false, {NOTE_FS4, 30}); }
    void playEnemyShot() { play(false, {NOTE_B5, 20, NOTE_F5, 30}); }
    void playSealed() { play(true, {NOTE_C3, 80, NOTE_C3, 80}); }
    void playEquip() { play(true, {NOTE_C6, 40, NOTE_E6, 40, NOTE_G6, 80}); }
    void playBag() { play(true, {NOTE_G5, 40, NOTE_C6, 60}); }
    void playPotion() { play(true, {NOTE_E6, 40, NOTE_B6, 60}); }
    void playDrink() { play(true, {NOTE_C5, 40, NOTE_E5, 40, NOTE_G5, 40}); }

    void playDescend()
    {
        play(true, {NOTE_G5, 70, NOTE_D5, 70, NOTE_AS4, 70, NOTE_G4, 70, NOTE_D4, 200});
    }

    void playBossDown()
    {
        play(true, {NOTE_C5, 80, NOTE_G4, 80, NOTE_DS5, 80, NOTE_C5, 80, NOTE_G5, 300});
    }

    void playDie()
    {
        play(true, {NOTE_A4, 140, NOTE_F4, 140, NOTE_D4, 140, NOTE_A3, 420});
    }

    // Under the title: a slow minor phrase, about 1.2 s
    void playTitle()
    {
        play(false, {NOTE_D4, 180, NOTE_F4, 180, NOTE_A4, 180, NOTE_GS4, 240, NOTE_D4, 420});
    }
};

#endif // SOUND_MANAGER_H
