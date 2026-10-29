#ifndef SONGS_H
#define SONGS_H

#include "../core/Buzzer.h"
#include "../pitches.h"
#include "../jack/Config.h"

// One loop per SONGS row, written for the buzzer after the moods of the Unity soundtrack
// (an 8-bit adventure, a rock riff, an orchestra loop, the Creepy and Neon themes)

namespace adventure
{
    static const uint16_t E = 170;
    static const Note NOTES[] = {
        {NOTE_C5, E}, {NOTE_E5, E}, {NOTE_G5, E}, {NOTE_C6, E}, {NOTE_B5, E}, {NOTE_G5, E}, {NOTE_E5, E}, {NOTE_G5, E},
        {NOTE_A5, E}, {NOTE_F5, E}, {NOTE_A5, E}, {NOTE_C6, E}, {NOTE_G5, 2 * E}, {NOTE_E5, 2 * E},
        {NOTE_F5, E}, {NOTE_A5, E}, {NOTE_C6, E}, {NOTE_F6, E}, {NOTE_E6, E}, {NOTE_C6, E}, {NOTE_G5, E}, {NOTE_E5, E},
        {NOTE_D5, E}, {NOTE_F5, E}, {NOTE_B5, E}, {NOTE_D6, E}, {NOTE_C6, 2 * E}, {0, 2 * E},
        {NOTE_E5, E}, {NOTE_G5, E}, {NOTE_C6, E}, {NOTE_E6, E}, {NOTE_D6, E}, {NOTE_B5, E}, {NOTE_G5, E}, {NOTE_B5, E},
        {NOTE_C6, E}, {NOTE_A5, E}, {NOTE_F5, E}, {NOTE_A5, E}, {NOTE_G5, 2 * E}, {NOTE_C5, 2 * E},
        {NOTE_F5, E}, {NOTE_E5, E}, {NOTE_D5, E}, {NOTE_C5, E}, {NOTE_B4, E}, {NOTE_C5, E}, {NOTE_D5, E}, {NOTE_E5, E},
        {NOTE_C5, 4 * E}, {0, 4 * E},
    };
}

namespace spike
{
    static const uint16_t E = 140;
    static const Note NOTES[] = {
        {NOTE_E4, E}, {NOTE_E4, E}, {NOTE_G4, E}, {NOTE_E4, E}, {NOTE_A4, E}, {NOTE_E4, E}, {NOTE_AS4, E}, {NOTE_A4, E},
        {NOTE_E4, E}, {NOTE_E4, E}, {NOTE_G4, E}, {NOTE_E4, E}, {NOTE_D5, E}, {NOTE_B4, E}, {NOTE_A4, E}, {NOTE_G4, E},
        {NOTE_A4, E}, {NOTE_A4, E}, {NOTE_C5, E}, {NOTE_A4, E}, {NOTE_D5, E}, {NOTE_A4, E}, {NOTE_DS5, E}, {NOTE_D5, E},
        {NOTE_B4, E}, {NOTE_B4, E}, {NOTE_D5, E}, {NOTE_B4, E}, {NOTE_FS5, E}, {NOTE_E5, E}, {NOTE_D5, E}, {NOTE_B4, E},
    };
}

namespace orchestra
{
    static const uint16_t E = 210;
    static const Note NOTES[] = {
        {NOTE_D5, 2 * E}, {NOTE_A4, E}, {NOTE_D5, E}, {NOTE_F5, 2 * E}, {NOTE_E5, E}, {NOTE_D5, E},
        {NOTE_C5, 2 * E}, {NOTE_G4, E}, {NOTE_C5, E}, {NOTE_E5, 2 * E}, {NOTE_D5, E}, {NOTE_C5, E},
        {NOTE_AS4, 2 * E}, {NOTE_F5, 2 * E}, {NOTE_A5, 2 * E}, {NOTE_G5, E}, {NOTE_F5, E},
        {NOTE_E5, 2 * E}, {NOTE_A4, 2 * E}, {NOTE_CS5, 2 * E}, {NOTE_E5, 2 * E},
        {NOTE_D5, 2 * E}, {NOTE_A4, E}, {NOTE_D5, E}, {NOTE_F5, 2 * E}, {NOTE_A5, 2 * E},
        {NOTE_AS5, 2 * E}, {NOTE_A5, E}, {NOTE_G5, E}, {NOTE_F5, 2 * E}, {NOTE_E5, 2 * E},
        {NOTE_D5, 3 * E}, {NOTE_CS5, E}, {NOTE_D5, 2 * E}, {NOTE_E5, 2 * E},
        {NOTE_D5, 6 * E}, {0, 2 * E},
    };
}

namespace haunted
{
    static const uint16_t E = 250;
    static const Note NOTES[] = {
        {NOTE_A4, E}, {NOTE_C5, E}, {NOTE_B4, E}, {NOTE_DS5, E},
        {NOTE_E5, 2 * E}, {0, E}, {NOTE_A4, E},
        {NOTE_F5, E}, {NOTE_E5, E}, {NOTE_DS5, E}, {NOTE_E5, E},
        {NOTE_C5, 2 * E}, {NOTE_B4, 2 * E},
        {NOTE_A4, E}, {NOTE_C5, E}, {NOTE_B4, E}, {NOTE_F5, E},
        {NOTE_E5, 3 * E}, {0, E},
        {NOTE_DS5, E}, {NOTE_E5, E}, {NOTE_F5, E}, {NOTE_GS4, E},
        {NOTE_A4, 3 * E}, {0, E},
    };
}

namespace neon
{
    static const uint16_t S = 110;
    static const Note NOTES[] = {
        {NOTE_A4, S}, {NOTE_C5, S}, {NOTE_E5, S}, {NOTE_A5, S}, {NOTE_E5, S}, {NOTE_C5, S},
        {NOTE_A4, S}, {NOTE_C5, S}, {NOTE_E5, S}, {NOTE_A5, S}, {NOTE_E5, S}, {NOTE_C5, S},
        {NOTE_F4, S}, {NOTE_A4, S}, {NOTE_C5, S}, {NOTE_F5, S}, {NOTE_C5, S}, {NOTE_A4, S},
        {NOTE_F4, S}, {NOTE_A4, S}, {NOTE_C5, S}, {NOTE_F5, S}, {NOTE_C5, S}, {NOTE_A4, S},
        {NOTE_C5, S}, {NOTE_E5, S}, {NOTE_G5, S}, {NOTE_C6, S}, {NOTE_G5, S}, {NOTE_E5, S},
        {NOTE_C5, S}, {NOTE_E5, S}, {NOTE_G5, S}, {NOTE_C6, S}, {NOTE_G5, S}, {NOTE_E5, S},
        {NOTE_G4, S}, {NOTE_B4, S}, {NOTE_D5, S}, {NOTE_G5, S}, {NOTE_D5, S}, {NOTE_B4, S},
        {NOTE_G4, S}, {NOTE_B4, S}, {NOTE_D5, S}, {NOTE_G5, S}, {NOTE_D5, S}, {NOTE_B4, S},
    };
}

struct Song
{
    const Note *notes;
    uint16_t count;
};

#define SONG(ns) {ns::NOTES, sizeof(ns::NOTES) / sizeof(Note)}
static const Song SONG_TRACKS[] = {SONG(adventure), SONG(spike), SONG(orchestra), SONG(haunted), SONG(neon)};
#undef SONG
static_assert(sizeof(SONG_TRACKS) / sizeof(Song) == SONG_COUNT, "One track per SONGS row");

#endif // SONGS_H
