#ifndef RENDER_FRAME_H
#define RENDER_FRAME_H

#include <Arduino.h>
#include <TFT_eSPI.h>
#include "View.h"

// Sends the view to the panel in bands: one band goes out by DMA while the next is drawn into the
// other buffer. The buffers are already byte-swapped, so the panel takes them as they are.
class Frame
{
public:
    static const int16_t BAND = 32;

    explicit Frame(TFT_eSPI &tft) : tft(tft) {}

    // The view's top-left corner on the panel is (0, 0)
    void draw(const View &view);
    // Runs between bands, so a slow frame does not miss a short button press
    void setPoll(void (*poll)()) { pollFn = poll; }

    uint32_t getDrawMicros() const { return drawMicros; }
    uint32_t getFrameMicros() const { return frameMicros; }

private:
    TFT_eSPI &tft;
    uint16_t buffers[2][View::W * BAND];
    void (*pollFn)() = nullptr;
    uint32_t drawMicros = 0;
    uint32_t frameMicros = 0;
};

#endif // RENDER_FRAME_H
