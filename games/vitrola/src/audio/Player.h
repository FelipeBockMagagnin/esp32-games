#ifndef PLAYER_H
#define PLAYER_H

#include <stdint.h>
#include "TrackReader.h"
#include "../music/Tracks.h"

// Plays one track on the passive buzzer. On the board an esp_timer calls tick() 120 times a
// second and drives LEDC channel 0 itself, at the launcher's volume: playTone()'s queue cannot keep
// 8 ms frames. On a PC nothing calls tick() but the caller, and nothing sounds.
class Player
{
public:
    static const uint8_t BANDS = 16;
    static const uint8_t LEVEL_MAX = 12;

    void begin(int pin);

    void play(uint16_t track);
    void setPaused(bool paused);
    void stop();

    bool isLoaded() const { return loaded; }
    bool isPaused() const { return paused; }
    bool isPlaying() const { return loaded && !paused; }
    uint16_t getTrack() const { return track; }
    uint32_t getFrame() const { return frame; }
    uint16_t getPeriod() const { return period; }
    // True once after a track ran out, for the playlist to pick the next
    bool takeEnded();
    // The level meter: each band of pitch jumps when sounded and falls back frame by frame
    uint8_t getLevel(uint8_t band) const { return levels[band]; }

    // One frame; public so a PC tool can drive it
    void tick();

private:
    TrackReader reader;
    volatile bool loaded = false;
    volatile bool paused = false;
    volatile bool ended = false;
    volatile uint16_t track = 0;
    volatile uint32_t frame = 0;
    volatile uint16_t period = 0;
    volatile uint8_t levels[BANDS] = {};
    int pin = -1;
    uint16_t duty = 0;
    uint16_t sounding = 0;
    uint32_t ticks = 0;

    void sound(uint16_t newPeriod);
};

#endif // PLAYER_H
