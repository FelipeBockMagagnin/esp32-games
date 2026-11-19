#ifndef TRACK_READER_H
#define TRACK_READER_H

#include <stdint.h>

// Steps through a track one 120 Hz frame at a time, as System Beeps' own player does: an event
// holds its period for its count of frames, a 255 holds the period one more frame
class TrackReader
{
public:
    void start(const uint8_t *data, uint32_t size)
    {
        this->data = data;
        this->size = size;
        pos = 1;
        wait = 0;
        period = 0;
        frame = 0;
        ended = data == nullptr || size < 2;
    }

    // The period to sound this frame (0 silence); false once the track has ended
    bool step(uint16_t &out)
    {
        if (ended)
        {
            out = 0;
            return false;
        }
        if (wait > 0)
        {
            wait--;
        }
        while (wait == 0)
        {
            if (pos >= size || data[pos] == 0)
            {
                ended = true;
                period = 0;
                out = 0;
                return false;
            }
            uint8_t w = data[pos++];
            if (w == 255)
            {
                break;
            }
            if (pos + 2 > size)
            {
                ended = true;
                out = 0;
                return false;
            }
            period = (uint16_t)(data[pos] | data[pos + 1] << 8);
            pos += 2;
            wait = w;
        }
        frame++;
        out = period;
        return true;
    }

    uint32_t getFrame() const { return frame; }
    bool hasEnded() const { return ended; }

private:
    const uint8_t *data = nullptr;
    uint32_t size = 0;
    uint32_t pos = 0;
    uint16_t wait = 0;
    uint16_t period = 0;
    uint32_t frame = 0;
    bool ended = true;
};

#endif // TRACK_READER_H
