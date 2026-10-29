#include "Frame.h"

void Frame::draw(const View &view)
{
    uint32_t start = micros(), drawn = 0;
    tft.startWrite();
    for (int16_t top = 0, band = 0; top < View::H; top += BAND, band++)
    {
        if (pollFn != nullptr)
        {
            pollFn();
        }
        int16_t rows = View::H - top < BAND ? View::H - top : BAND;
        uint16_t *buffer = buffers[band & 1];
        uint32_t t0 = micros();
        view.drawBand(buffer, top, rows);
        drawn += micros() - t0;
        // Waits for the band before last, which used the other buffer
        tft.pushImageDMA(0, top, View::W, rows, buffer);
    }
    tft.dmaWait();
    tft.endWrite();
    drawMicros = drawn;
    frameMicros = micros() - start;
}
