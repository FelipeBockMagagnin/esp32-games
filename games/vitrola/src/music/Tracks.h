#ifndef TRACKS_H
#define TRACKS_H

#include <stdint.h>

// Every track is a PC speaker stream in System Beeps' format (tools/tracks.py has it in full):
// a header byte, then [frames, period lo, period hi] events at 120 frames a second, 0 at the end
static const uint8_t FRAME_RATE = 120;
static const uint32_t PIT_CLOCK = 1193180;

struct Album
{
    const char *name;
    const char *artist;
    const char *label1; // The two lines printed on the record's label
    const char *label2;
    uint16_t color; // The label's paper and ink, RGB565
    uint16_t ink;
    uint8_t first; // Its tracks are TRACKS[first .. first + count)
    uint8_t count;
    uint32_t frames;
};

struct Track
{
    const char *title;
    const char *shelf; // The title, or a shorter one where it would not fit the shelf's row
    const char *author;
    uint8_t album;
    const uint8_t *data;
    uint32_t size;
    uint32_t frames;
};

extern const Album ALBUMS[];
extern const uint8_t ALBUM_COUNT;
extern const Track TRACKS[];
extern const uint16_t TRACK_COUNT;

#endif // TRACKS_H
