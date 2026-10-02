#include "ButtonBadge.h"

// Colors of the physical caps, plus dull versions for an unavailable action
static const uint16_t CONFIRM_CAP = 0xE8E4; // Red
static const uint16_t CONFIRM_CAP_DIM = 0x5000;
static const uint16_t ARROW_CAP = 0xFFFF; // White
static const uint16_t ARROW_CAP_DIM = 0x52AA;
static const uint16_t ARROW_COLOR = 0x0000;
static const uint16_t RIM_COLOR = 0x8410; // Gray outline so the cap reads as a button

void ButtonBadge::draw(TFT_eSPI &tft)
{
    int16_t r = SIZE / 2;
    int16_t cx = x + r;
    int16_t cy = y + r;

    if (kind == CONFIRM)
    {
        tft.fillCircle(cx, cy, r, dimmed ? CONFIRM_CAP_DIM : CONFIRM_CAP);
        tft.drawCircle(cx, cy, r, RIM_COLOR);
        return;
    }

    tft.fillCircle(cx, cy, r, dimmed ? ARROW_CAP_DIM : ARROW_CAP);
    tft.drawCircle(cx, cy, r, RIM_COLOR);
    if (kind == UP)
    {
        tft.fillTriangle(cx, cy - 3, cx - 3, cy + 2, cx + 3, cy + 2, ARROW_COLOR);
    }
    else
    {
        tft.fillTriangle(cx, cy + 3, cx - 3, cy - 2, cx + 3, cy - 2, ARROW_COLOR);
    }
}
