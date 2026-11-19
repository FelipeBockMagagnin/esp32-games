#include "VuMeter.h"
#include "Theme.h"

static const int16_t SEG_H = 4;

void VuMeter::reset()
{
    for (uint8_t b = 0; b < Player::BANDS; b++)
    {
        drawn[b] = 255;
    }
}

void VuMeter::draw(TFT_eSPI &tft, const Player &player)
{
    for (uint8_t b = 0; b < Player::BANDS; b++)
    {
        uint8_t lit = (uint8_t)((player.getLevel(b) * SEGMENTS + Player::LEVEL_MAX - 1) / Player::LEVEL_MAX);
        if (lit == drawn[b])
        {
            continue;
        }
        for (uint8_t s = 0; s < SEGMENTS; s++)
        {
            bool was = drawn[b] != 255 && s < drawn[b];
            bool on = s < lit;
            if (drawn[b] != 255 && was == on)
            {
                continue;
            }
            Color c = !on ? Theme::VU_OFF : s >= SEGMENTS - 1 ? Theme::VU_HIGH : s >= SEGMENTS - 3 ? Theme::VU_MID : Theme::VU_LOW;
            tft.fillRect(X0 + b * PITCH, Y + H - (s + 1) * SEG_H, BAR_W, SEG_H - 1, c);
        }
        drawn[b] = lit;
    }
}
