#ifndef BUZZER_H
#define BUZZER_H

#include <Arduino.h>

struct Note
{
    uint16_t freq; // 0 rests
    uint16_t ms;
};

// The passive buzzer, driven by a task of its own on LEDC channel 0 so a song keeps time
// while the loop pushes frames. A sound effect cuts in at once over the song, which keeps
// counting underneath and comes back on the beat. tone() is not used: its queue plays notes
// one after the other, so an effect would wait for the song's note to end.
class Buzzer
{
public:
    static const uint8_t MAX_EFFECT = 8;

    void begin(int pin);
    // Replaces the effect playing, if any
    void playEffect(const Note *notes, uint8_t count);
    // Loops until changed; nullptr stops it
    void playSong(const Note *notes, uint16_t count);

private:
    static const uint8_t CHANNEL = 0;
    static const uint16_t GAP_MS = 12; // Silence at the end of each song note, so repeats do not slur

    portMUX_TYPE lock = portMUX_INITIALIZER_UNLOCKED;
    Note pending[MAX_EFFECT];
    uint8_t pendingCount = 0;
    bool effectWaiting = false;
    const Note *nextSong = nullptr;
    uint16_t nextSongCount = 0;
    bool songWaiting = false;

    // Only the task touches these
    Note effect[MAX_EFFECT];
    uint8_t effectCount = 0;
    uint8_t effectPos = 0;
    unsigned long effectEnd = 0;
    const Note *song = nullptr;
    uint16_t songCount = 0;
    uint16_t songPos = 0;
    unsigned long songEnd = 0;
    uint16_t sounding = 0;

    static void taskEntry(void *self);
    void run();
    void sound(uint16_t freq);
};

#endif // BUZZER_H
