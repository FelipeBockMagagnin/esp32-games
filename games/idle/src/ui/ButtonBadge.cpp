#include "ButtonBadge.h"
#include "Theme.h"

// Colors of the physical caps, plus dull versions for an unavailable action
static const uint16_t CONFIRM_CAP = Theme::CONFIRM;
static const uint16_t CONFIRM_CAP_DIM = Theme::CONFIRM_DIM;
static const uint16_t ARROW_CAP = Theme::TEXT;
static const uint16_t ARROW_CAP_DIM = Theme::TEXT_DIM;
static const uint16_t ARROW_COLOR = Theme::BG;
static const uint16_t RIM_COLOR = Theme::TEXT_2; // Outline so the cap reads as a button

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
