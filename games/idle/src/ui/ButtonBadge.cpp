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
    drawCap(tft, x + r, y + r, r, kind, dimmed);
}

void ButtonBadge::drawCap(TFT_eSPI &tft, int16_t cx, int16_t cy, int16_t r, Kind kind, bool dimmed)
{
    if (kind == CONFIRM)
    {
        tft.fillCircle(cx, cy, r, dimmed ? CONFIRM_CAP_DIM : CONFIRM_CAP);
        tft.drawCircle(cx, cy, r, RIM_COLOR);
        return;
    }

    tft.fillCircle(cx, cy, r, dimmed ? ARROW_CAP_DIM : ARROW_CAP);
    tft.drawCircle(cx, cy, r, RIM_COLOR);
    int16_t a = r - 2; // Arrow half-size: 3 on the 11px badge
    if (kind == UP)
    {
        tft.fillTriangle(cx, cy - a, cx - a, cy + a - 1, cx + a, cy + a - 1, ARROW_COLOR);
    }
    else
    {
        tft.fillTriangle(cx, cy + a, cx - a, cy - a + 1, cx + a, cy - a + 1, ARROW_COLOR);
    }
}
