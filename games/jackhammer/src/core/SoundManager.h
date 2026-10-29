#ifndef SOUND_MANAGER_H
#define SOUND_MANAGER_H

#include <Arduino.h>
#include "Buzzer.h"
#include "pitches.h"

enum class SoundMode : uint8_t
{
    ON,
    QUIET, // Effects without the music
    OFF,
    COUNT
};

class SoundManager
{
public:
    void begin(int pin) { buzzer.begin(pin); }

    void setMode(SoundMode m)
    {
        if (m == mode)
        {
            return;
        }
        mode = m;
        applySong();
    }
    SoundMode getMode() const { return mode; }

    void setSong(const Note *notes, uint16_t count)
    {
        if (notes == songNotes && count == songCount)
        {
            return;
        }
        songNotes = notes;
        songCount = count;
        applySong();
    }

    void playMenu() { effect({{NOTE_GS7, 60}}); }
    void playError() { effect({{NOTE_A3, 70}}); }
    void playTurn() { effect({{NOTE_C7, 12}, {NOTE_G6, 12}}); }

    // A semitone either way at random, so a run of kills does not drone
    void playKill()
    {
        float shift = powf(2.0f, (random(3) - 1) / 12.0f);
        effect({{(uint16_t)(NOTE_G5 * shift), 25}, {(uint16_t)(NOTE_C5 * shift), 30}, {(uint16_t)(NOTE_C7 * shift), 35}});
    }

    void playDeath() { effect({{NOTE_D4, 140}, {NOTE_A3, 140}, {NOTE_F3, 140}, {NOTE_D3, 420}}); }
    void playStart() { effect({{NOTE_C5, 80}, {NOTE_G5, 80}, {NOTE_C6, 80}, {NOTE_E6, 200}}); }
    void playBuy() { effect({{NOTE_C6, 60}, {NOTE_G6, 60}, {NOTE_C7, 60}, {NOTE_E7, 60}, {NOTE_G7, 180}}); }
    void playBest() { effect({{NOTE_C6, 80}, {NOTE_E6, 80}, {NOTE_G6, 80}, {NOTE_C7, 240}}); }

private:
    Buzzer buzzer;
    SoundMode mode = SoundMode::ON;
    const Note *songNotes = nullptr;
    uint16_t songCount = 0;

    void effect(std::initializer_list<Note> notes)
    {
        if (mode != SoundMode::OFF)
        {
            buzzer.playEffect(notes.begin(), (uint8_t)notes.size());
        }
    }

    void applySong()
    {
        if (mode == SoundMode::ON)
        {
            buzzer.playSong(songNotes, songCount);
        }
        else
        {
            buzzer.playSong(nullptr, 0);
        }
    }
};

#endif // SOUND_MANAGER_H
