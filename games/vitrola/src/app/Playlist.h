#ifndef PLAYLIST_H
#define PLAYLIST_H

#include <stdint.h>
#include "../music/Tracks.h"

enum class Mode : uint8_t
{
    ALBUM,  // The record's side to its end, then the arm lifts
    REPEAT, // The record over and over
    ONE,    // The same track over and over
    SHUFFLE,
    COUNT
};

const char *modeName(Mode m);
const char *modeTag(Mode m); // Three letters for the display

// Which track comes next, by hand or once one ends
class Playlist
{
public:
    explicit Playlist(uint32_t (*random)()) : random(random) {}

    uint16_t current = 0;
    Mode mode = Mode::ALBUM;

    uint8_t album() const { return TRACKS[current].album; }
    // Position within its album, from 0
    uint8_t index() const { return current - ALBUMS[album()].first; }
    // Frames of its album before it, for where the needle sits
    uint32_t framesBefore() const;

    // The arrows step through the album, round from one end to the other; in shuffle they draw
    uint16_t next() const;
    uint16_t previous() const;
    // -1 stops
    int32_t afterEnd() const;

private:
    uint32_t (*random)();
    uint16_t draw() const;
};

#endif // PLAYLIST_H
