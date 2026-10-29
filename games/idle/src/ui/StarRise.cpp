#include "StarRise.h"

static const unsigned long FRAME_MS = 30;
static constexpr Color PALETTE[] = {Theme::GOLD, Theme::EPIC, Theme::TEXT, Theme::MYTHIC};
static const uint8_t PALETTE_SIZE = sizeof(PALETTE) / sizeof(PALETTE[0]);

StarRise::StarRise(int16_t x, int16_t y, int16_t w, int16_t h)
    : UI(x, y, w, h), lastFrame(0), running(false)
{
    for (Star &star : stars)
    {
        star.drawnY = -1;
    }
}

void StarRise::respawn(Star &star)
{
    star.x = (int16_t)random(w - DOT);
    star.y = h - DOT;
    star.speed = (uint8_t)random(1, 5);
    star.color = (uint8_t)random(PALETTE_SIZE);
}

void StarRise::start(unsigned long now)
{
    // Spread over the whole height at first, so the area does not fill from the bottom
    for (Star &star : stars)
    {
        respawn(star);
        star.y = (int16_t)random(h - DOT);
        star.drawnY = -1;
    }
    lastFrame = now;
    running = true;
    markDirty();
}

void StarRise::update(unsigned long now)
{
    if (!running || now - lastFrame < FRAME_MS)
    {
        return;
    }
    lastFrame = now;
    for (Star &star : stars)
    {
        star.y -= star.speed;
        if (star.y < 0)
        {
            respawn(star);
        }
    }
    markDirty();
}

void StarRise::draw(TFT_eSPI &tft)
{
    for (Star &star : stars)
    {
        if (star.drawnY >= 0)
        {
            tft.fillRect(x + star.drawnX, y + star.drawnY, DOT, DOT, eraseColor);
        }
        if (!running)
        {
            star.drawnY = -1;
            continue;
        }
        tft.fillRect(x + star.x, y + star.y, DOT, DOT, PALETTE[star.color]);
        star.drawnX = star.x;
        star.drawnY = star.y;
    }
}
