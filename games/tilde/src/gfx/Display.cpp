#include "Display.h"

void Display::begin()
{
    tft.initDMA();
    // pushImageDMA swaps the buffer in place into the panel's byte order
    tft.setSwapBytes(true);
}

void Display::frame(const Drawable &scene, uint16_t background)
{
    uint32_t start = micros(), drawn = 0;
    int16_t edges[Drawable::MAX_SEGMENTS + 1];
    uint8_t count = scene.segments(edges);
    // Another screen's cuts: what was sent under the old ones says nothing about the new
    bool same = count == segmentCount;
    for (uint8_t i = 0; i <= count && same; i++)
    {
        same = edges[i] == lastEdges[i];
    }
    if (!same)
    {
        valid = false;
        memcpy(lastEdges, edges, sizeof(int16_t) * (count + 1));
        segmentCount = count;
    }
    segmentsSent = 0;
    tft.startWrite();
    for (uint8_t i = 0; i < count; i++)
    {
        if (pollFn != nullptr)
        {
            pollFn();
        }
        uint32_t key = scene.bandKey(i);
        if (valid && key != 0 && key == keys[i])
        {
            continue;
        }
        keys[i] = key;
        int16_t top = edges[i], rows = edges[i + 1] - edges[i];
        rows = rows > MAX_ROWS ? MAX_ROWS : rows;
        // The one buffer may still be going out
        tft.dmaWait();
        uint32_t t0 = micros();
        canvas.begin(buffer, top, rows);
        canvas.clear(background);
        scene.draw(canvas);
        // FNV-1a over the segment, before the swap changes it
        uint32_t sum = 2166136261u;
        const uint32_t *words = (const uint32_t *)buffer;
        for (int32_t w = 0; w < Canvas::W * rows / 2; w++)
        {
            sum = (sum ^ words[w]) * 16777619u;
        }
        segmentMicros[i] = micros() - t0;
        drawn += segmentMicros[i];
        if (valid && sum == sums[i])
        {
            continue;
        }
        sums[i] = sum;
        tft.pushImageDMA(0, top, Canvas::W, rows, buffer);
        segmentsSent++;
    }
    tft.dmaWait();
    tft.endWrite();
    valid = true;
    drawMicros = drawn;
    frameMicros = micros() - start;
}
