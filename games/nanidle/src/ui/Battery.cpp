#include "Battery.h"
#include "Win.h"

static const int16_t NUB_W = 4;
static const int16_t EDGE = 2;

void Battery::setLevels(double allocated, double idle)
{
    int16_t inner = w - NUB_W - 2 * EDGE - 2;
    auto px = [inner](double f) { return (int16_t)(inner * (f > 1 ? 1 : (f > 0 ? f : 0))); };
    int16_t a = px(allocated);
    int16_t i = px(allocated + idle) - a;
    if (a != allocatedPx || i != idlePx)
    {
        allocatedPx = a;
        idlePx = i;
        markDirty();
    }
}

void Battery::setLabel(const char *text)
{
    if (strcmp(label, text) != 0)
    {
        strncpy(label, text, sizeof(label) - 1);
        markDirty();
    }
}

void Battery::draw(TFT_eSPI &tft)
{
    int16_t bodyW = w - NUB_W;
    tft.fillRoundRect(x, y, bodyW, h, 4, Theme::EDGE_DARK);
    tft.fillRect(x + bodyW, y + h / 3, NUB_W, h - 2 * (h / 3), Theme::EDGE_DARK);
    int16_t ix = x + EDGE + 1, iy = y + EDGE + 1, iw = bodyW - 2 * EDGE - 2, ih = h - 2 * EDGE - 2;
    tft.fillRect(ix, iy, iw, ih, Theme::FIELD);
    if (allocatedPx > 0)
    {
        Win::gradientV(tft, ix, iy, allocatedPx, ih, Theme::BAR_HI, Theme::BAR);
    }
    if (idlePx > 0)
    {
        Win::gradientV(tft, ix + allocatedPx, iy, idlePx, ih, Theme::FIELD, Theme::GOOD);
    }
    // A glass highlight across the top
    tft.drawFastHLine(ix + 2, iy + 2, iw - 4, tft.alphaBlend(120, Theme::FIELD, Theme::BAR_HI));

    tft.setTextSize(1);
    tft.setTextDatum(MC_DATUM);
    tft.setTextColor(Theme::EDGE_DARK);
    tft.drawString(label, x + bodyW / 2 + 1, y + h / 2 + 2);
    tft.setTextColor(Theme::FIELD);
    tft.drawString(label, x + bodyW / 2, y + h / 2 + 1);
    tft.setTextDatum(TL_DATUM);
}
