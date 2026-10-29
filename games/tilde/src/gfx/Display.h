#ifndef GFX_DISPLAY_H
#define GFX_DISPLAY_H

#include <Arduino.h>
#include <TFT_eSPI.h>
#include "Canvas.h"

// Anything that draws a whole frame. draw() runs once per segment, so it must only read state:
// whatever animates advances in update(), never in draw().
class Drawable
{
public:
    static const uint8_t MAX_SEGMENTS = 6;

    virtual ~Drawable() = default;
    virtual void draw(Canvas &canvas) const = 0;
    // Where the frame is cut into segments, each drawn and sent whole: fills `edges` with the
    // rows from 0 to Canvas::H and returns how many segments that makes. Something that moves
    // must sit inside one segment, or the panel shows its top from one frame and its bottom
    // from the last while the frame goes out. None may pass Display::MAX_ROWS.
    virtual uint8_t segments(int16_t *edges) const
    {
        edges[0] = 0;
        edges[1] = 32;
        edges[2] = 216;
        edges[3] = Canvas::H;
        return 3;
    }
    // A cheap fingerprint of what a segment shows, worked out from state rather than pixels: when
    // it matches the last one drawn, the segment is neither drawn nor sent. 0 means always draw.
    virtual uint32_t bandKey(uint8_t segment) const { return 0; }
};

// Draws a frame a segment at a time into one buffer and sends each segment in one DMA transfer.
// A segment identical to the one last sent is skipped, so a still part of the screen costs no SPI.
class Display
{
public:
    static const int16_t MAX_ROWS = 184;

    explicit Display(TFT_eSPI &tft) : tft(tft) {}

    void begin();
    // The next frame sends every segment (after a theme change, or a modal over the screen)
    void invalidate() { valid = false; }
    void frame(const Drawable &scene, uint16_t background);
    // Runs between segments, so a slow frame does not miss a short button press
    void setPoll(void (*poll)()) { pollFn = poll; }

    uint32_t getDrawMicros() const { return drawMicros; }
    uint32_t getFrameMicros() const { return frameMicros; }
    uint8_t getSegmentsSent() const { return segmentsSent; }
    uint8_t getSegmentCount() const { return segmentCount; }
    // A segment's drawing time in the last frame it was drawn
    uint32_t getSegmentMicros(uint8_t segment) const { return segmentMicros[segment]; }

private:
    TFT_eSPI &tft;
    Canvas canvas;
    uint16_t buffer[Canvas::W * MAX_ROWS];
    uint32_t sums[Drawable::MAX_SEGMENTS] = {};
    uint32_t keys[Drawable::MAX_SEGMENTS] = {};
    int16_t lastEdges[Drawable::MAX_SEGMENTS + 1] = {};
    void (*pollFn)() = nullptr;
    bool valid = false;
    uint32_t drawMicros = 0;
    uint32_t frameMicros = 0;
    uint8_t segmentsSent = 0;
    uint8_t segmentCount = 0;
    uint32_t segmentMicros[Drawable::MAX_SEGMENTS] = {};
};

#endif // GFX_DISPLAY_H
