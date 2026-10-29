// The panel as a framebuffer, so tools/shots.cpp can write what the screens drew as a PNG
#ifndef STUB_TFT_ESPI_H
#define STUB_TFT_ESPI_H

#include <Arduino.h>

#ifndef TFT_WIDTH
#define TFT_WIDTH 240
#define TFT_HEIGHT 320
#endif

class TFT_eSPI
{
public:
    uint16_t pixels[TFT_WIDTH * TFT_HEIGHT] = {};
    unsigned long pushes = 0;

    void setSwapBytes(bool) {}
    bool initDMA(bool = false) { return true; }
    void startWrite() {}
    void endWrite() {}
    void dmaWait() {}
    void pushImageDMA(int32_t x, int32_t y, int32_t w, int32_t h, uint16_t *data) { pushImage(x, y, w, h, data); }
    void fillScreen(uint16_t color)
    {
        for (auto &p : pixels)
        {
            p = color;
        }
    }
    void pushImage(int32_t x, int32_t y, int32_t w, int32_t h, const uint16_t *data)
    {
        pushes++;
        for (int32_t row = 0; row < h; row++)
        {
            for (int32_t col = 0; col < w; col++)
            {
                if (x + col >= 0 && x + col < TFT_WIDTH && y + row >= 0 && y + row < TFT_HEIGHT)
                {
                    pixels[(y + row) * TFT_WIDTH + x + col] = data[row * w + col];
                }
            }
        }
    }
};

#endif // STUB_TFT_ESPI_H
