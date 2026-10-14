#include "ScreenWipe.h"

static const unsigned long WIPE_MS = 420;
static const unsigned long FRAME_MS = 20;

ScreenWipe::ScreenWipe(uint16_t color)
    : UI(0, 0, TFT_WIDTH, TFT_HEIGHT), color(color), startTime(0), reach(0), drawnReach(-1), done(false),
      lastFrame(0)
{
    visible = false;
}

void ScreenWipe::start(unsigned long now)
{
    startTime = now;
    reach = 0;
    drawnReach = -1;
    done = false;
    setVisible(true);
}

void ScreenWipe::stop()
{
    done = false;
    setVisible(false);
}

void ScreenWipe::update(unsigned long now)
{
    if (!visible || done || now - lastFrame < FRAME_MS)
    {
        return;
    }
    lastFrame = now;
    unsigned long age = now - startTime;
    int16_t half = h / 2;
    // Eased out, so the light bursts from the middle and slows toward the edges
    int32_t t = age < WIPE_MS ? (int32_t)age * 256 / WIPE_MS : 256;
    t = 256 - (256 - t) * (256 - t) / 256;
    reach = (int16_t)(half * t / 256);
    markDirty();
}

void ScreenWipe::draw(TFT_eSPI &tft)
{
    int16_t half = h / 2;
    if (reach >= half)
    {
        tft.fillScreen(Theme::TEXT);
        done = true;
        return;
    }
    for (int16_t r = drawnReach + 1; r <= reach; r++)
    {
        uint16_t c = tft.alphaBlend((uint8_t)(255 * r / half), Theme::TEXT, color);
        tft.drawFastHLine(x, y + half - 1 - r, w, c);
        tft.drawFastHLine(x, y + half + r, w, c);
    }
    drawnReach = reach;
}
