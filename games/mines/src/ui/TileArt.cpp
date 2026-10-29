#include "TileArt.h"
#include "Themes.h"

// 6x8, a row per byte with the leftmost dot in bit 5
static const uint8_t DIGITS[8][8] = {
    {0x0C, 0x1C, 0x3C, 0x0C, 0x0C, 0x0C, 0x0C, 0x3F}, // 1
    {0x1E, 0x33, 0x03, 0x06, 0x0C, 0x18, 0x30, 0x3F}, // 2
    {0x3E, 0x03, 0x03, 0x0E, 0x03, 0x03, 0x03, 0x3E}, // 3
    {0x06, 0x0E, 0x1E, 0x36, 0x3F, 0x06, 0x06, 0x06}, // 4
    {0x3F, 0x30, 0x30, 0x3E, 0x03, 0x03, 0x33, 0x1E}, // 5
    {0x1E, 0x30, 0x30, 0x3E, 0x33, 0x33, 0x33, 0x1E}, // 6
    {0x3F, 0x03, 0x06, 0x06, 0x0C, 0x0C, 0x18, 0x18}, // 7
    {0x1E, 0x33, 0x33, 0x1E, 0x33, 0x33, 0x33, 0x1E}, // 8
};

// The ring's dots clockwise from twelve o'clock, as hundredths of the radius
static const int8_t RING[TileArt::RING_DOTS][2] = {
    {0, -100}, {50, -87}, {87, -50}, {100, 0}, {87, 50}, {50, 87},
    {0, 100}, {-50, 87}, {-87, 50}, {-100, 0}, {-87, -50}, {-50, -87},
};

static TileStyle style()
{
    return activeTheme().style;
}

static void digit(TFT_eSPI &tft, int16_t x, int16_t y, int16_t s, uint8_t n, uint16_t color)
{
    int16_t scale = s >= 22 ? 2 : 1;
    int16_t ox = x + (s - 6 * scale) / 2, oy = y + (s - 8 * scale) / 2;
    for (int16_t r = 0; r < 8; r++)
    {
        uint8_t bits = DIGITS[n - 1][r];
        for (int16_t c = 0; c < 6; c++)
        {
            if (bits & (0x20 >> c))
            {
                tft.fillRect(ox + c * scale, oy + r * scale, scale, scale, color);
            }
        }
    }
}

static void hatch(TFT_eSPI &tft, int16_t x, int16_t y, int16_t s, uint16_t color)
{
    for (int16_t k = 4; k < 2 * s - 4; k += 4)
    {
        int16_t x0 = k < s - 2 ? k : s - 2, y0 = k < s - 2 ? 1 : k - (s - 2) + 1;
        int16_t x1 = k < s - 2 ? 1 : k - (s - 2) + 1, y1 = k < s - 2 ? k : s - 2;
        tft.drawLine(x + x0, y + y0, x + x1, y + y1, color);
    }
}

void TileArt::covered(TFT_eSPI &tft, int16_t x, int16_t y, int16_t s, uint8_t parity)
{
    uint16_t cover = Theme::COVER, hi = Theme::COVER_HI, lo = Theme::COVER_LO;
    int16_t e = s >= 22 ? 3 : 2;
    switch (style())
    {
    case TileStyle::BEVEL:
        tft.fillRect(x, y, s, s, cover);
        tft.fillRect(x, y, s, e, hi);
        tft.fillRect(x, y, e, s, hi);
        tft.fillRect(x, y + s - e, s, e, lo);
        tft.fillRect(x + s - e, y, e, s, lo);
        for (int16_t i = 0; i < e; i++)
        {
            tft.drawFastHLine(x + s - e, y + i, e - i, hi);
            tft.drawFastHLine(x, y + s - e + i, e - i, hi);
        }
        break;
    case TileStyle::FLAT:
        tft.fillRect(x, y, s, s, Theme::FIELD);
        tft.fillRoundRect(x + 1, y + 2, s - 2, s - 2, 3, lo);
        tft.fillRoundRect(x + 1, y + 1, s - 2, s - 3, 3, cover);
        tft.fillRect(x + 4, y + 3, s / 3, 2, hi);
        break;
    case TileStyle::GLOW:
        tft.fillRect(x, y, s, s, Theme::FIELD);
        tft.fillRoundRect(x + 2, y + 2, s - 4, s - 4, 2, cover);
        tft.drawRoundRect(x + 1, y + 1, s - 2, s - 2, 3, hi);
        tft.drawFastHLine(x + 4, y + s - 4, s - 8, lo);
        break;
    case TileStyle::SKETCH:
        tft.fillRect(x, y, s, s, Theme::OPEN);
        tft.drawFastHLine(x, y, s, Theme::OPEN_LINE);
        tft.drawFastVLine(x, y, s, Theme::OPEN_LINE);
        tft.fillRect(x + 1, y + 1, s - 2, s - 2, cover);
        hatch(tft, x, y, s, lo);
        tft.drawFastHLine(x + 1 + (parity & 1), y + 1, s - 3, lo);
        tft.drawFastHLine(x + 1, y + s - 2, s - 2 - (parity & 1), lo);
        tft.drawFastVLine(x + 1, y + 1 + ((parity >> 1) & 1), s - 3, lo);
        tft.drawFastVLine(x + s - 2, y + 1, s - 2, lo);
        break;
    case TileStyle::GRASS:
    {
        tft.fillRect(x, y, s, s, cover);
        tft.fillRect(x, y + s - 2, s, 2, lo);
        tft.drawFastVLine(x + s - 1, y, s, lo);
        int16_t tx = x + 3 + (parity % 3) * (s / 5), ty = y + s / 3 + (parity % 2) * (s / 4);
        tft.drawFastVLine(tx, ty + 1, 3, hi);
        tft.drawFastVLine(tx + 2, ty, 4, hi);
        tft.drawFastVLine(tx + 4, ty + 1, 3, hi);
        if (s >= 18)
        {
            int16_t ux = x + s - 7 - (parity % 2) * 3, uy = y + 3 + (parity % 3);
            tft.drawFastVLine(ux, uy + 1, 2, hi);
            tft.drawFastVLine(ux + 2, uy, 3, hi);
        }
        break;
    }
    case TileStyle::WAVE:
    {
        tft.fillRect(x, y, s, s, cover);
        tft.fillRect(x, y + s - 2, s, 2, lo);
        tft.drawFastVLine(x + s - 1, y, s, lo);
        int16_t wy = y + s / 3 + (parity & 1) * 2, wx = x + 2 + (parity % 3);
        tft.drawFastHLine(wx, wy + 1, 2, hi);
        tft.drawFastHLine(wx + 2, wy, 3, hi);
        tft.drawFastHLine(wx + 5, wy + 1, 2, hi);
        if (s >= 18)
        {
            wy += s / 3;
            wx = x + s / 2 - (parity % 2) * 2;
            tft.drawFastHLine(wx, wy + 1, 2, hi);
            tft.drawFastHLine(wx + 2, wy, 3, hi);
        }
        break;
    }
    }
}

void TileArt::open(TFT_eSPI &tft, int16_t x, int16_t y, int16_t s, uint8_t parity, uint8_t count)
{
    uint16_t open = Theme::OPEN, line = Theme::OPEN_LINE;
    tft.fillRect(x, y, s, s, open);
    switch (style())
    {
    case TileStyle::GLOW:
        tft.drawPixel(x, y, line);
        tft.drawPixel(x + 1, y, line);
        tft.drawPixel(x, y + 1, line);
        break;
    case TileStyle::GRASS:
    case TileStyle::WAVE:
        tft.drawFastHLine(x, y, s, line);
        tft.drawFastVLine(x, y, s, line);
        if (count == 0)
        {
            tft.fillRect(x + 3 + (parity % 4) * (s / 6), y + s - 5 - (parity % 3) * (s / 5), 2, 1, line);
            tft.drawPixel(x + s - 4 - (parity % 2) * (s / 3), y + 4 + (parity % 2) * 2, line);
        }
        break;
    default:
        tft.drawFastHLine(x, y, s, line);
        tft.drawFastVLine(x, y, s, line);
        break;
    }
    if (count > 0)
    {
        digit(tft, x + 1, y + 1, s - 1, count, Theme::number(count));
    }
}

void TileArt::popping(TFT_eSPI &tft, int16_t x, int16_t y, int16_t s, uint8_t parity, uint8_t frame)
{
    open(tft, x, y, s, parity, 0);
    int16_t inset = s * (frame + 1) / 6;
    tft.fillRect(x + inset, y + inset, s - 2 * inset, s - 2 * inset, Theme::COVER);
    tft.drawFastHLine(x + inset, y + inset, s - 2 * inset, Theme::COVER_HI);
}

void TileArt::flagGlyph(TFT_eSPI &tft, int16_t x, int16_t y, int16_t s, uint8_t frame)
{
    uint16_t pole = Theme::number(7);
    int16_t px = x + s * 9 / 16, top = y + s * 3 / 16, foot = y + s * 12 / 16;
    tft.fillRect(px, top, s >= 22 ? 2 : 1, foot - top, pole);
    tft.fillRect(x + s * 5 / 16, foot, s * 8 / 16, 2, pole);
    int16_t h = s * 6 / 16, w = s * 7 / 16;
    int16_t drop = (FLAG_FRAMES - 1 - frame) * (foot - top - h) / (FLAG_FRAMES - 1);
    int16_t py = top + drop;
    tft.fillTriangle(px - 1, py, px - 1, py + h, px - 1 - w, py + h / 2, Theme::FLAG);
}

void TileArt::flag(TFT_eSPI &tft, int16_t x, int16_t y, int16_t s, uint8_t parity, uint8_t frame)
{
    covered(tft, x, y, s, parity);
    flagGlyph(tft, x, y, s, frame);
}

void TileArt::wrongFlag(TFT_eSPI &tft, int16_t x, int16_t y, int16_t s, uint8_t parity)
{
    flag(tft, x, y, s, parity, FLAG_FRAMES - 1);
    uint16_t blast = Theme::BLAST;
    for (int16_t d = 0; d < 2; d++)
    {
        tft.drawLine(x + 3 + d, y + 3, x + s - 4 + d, y + s - 4, blast);
        tft.drawLine(x + s - 4 + d, y + 3, x + 3 + d, y + s - 4, blast);
    }
}

void TileArt::mineGlyph(TFT_eSPI &tft, int16_t cx, int16_t cy, int16_t s)
{
    uint16_t body = Theme::MINE, hi = Theme::MINE_HI;
    switch (activeTheme().glyph)
    {
    case MineGlyph::BOMB:
    {
        int16_t r = s * 5 / 16;
        tft.fillCircle(cx, cy + 1, r, body);
        tft.fillRect(cx + r / 2, cy - r, r / 2 + 1, r / 2 + 1, body);
        tft.drawLine(cx + r, cy - r, cx + r + 2, cy - r - 2, body);
        tft.drawPixel(cx + r + 3, cy - r - 3, Theme::CURSOR);
        tft.fillRect(cx - r / 2, cy - r / 2 + 1, 2, 2, hi);
        break;
    }
    case MineGlyph::FLOWER:
    {
        int16_t d = s / 5, r = s / 7 + 1;
        const int8_t P[5][2] = {{0, -10}, {10, -3}, {6, 8}, {-6, 8}, {-10, -3}};
        for (const auto &p : P)
        {
            tft.fillCircle(cx + p[0] * d / 10, cy + p[1] * d / 10, r, body);
        }
        tft.fillCircle(cx, cy, s / 9 + 1, hi);
        break;
    }
    case MineGlyph::URCHIN:
    {
        int16_t r = s / 5, l = s * 7 / 16;
        const int8_t D[8][2] = {{10, 0}, {7, 7}, {0, 10}, {-7, 7}, {-10, 0}, {-7, -7}, {0, -10}, {7, -7}};
        for (const auto &v : D)
        {
            tft.drawLine(cx, cy, cx + v[0] * l / 10, cy + v[1] * l / 10, body);
        }
        tft.fillCircle(cx, cy, r, body);
        tft.fillRect(cx - r / 2, cy - r / 2, 2, 2, hi);
        break;
    }
    case MineGlyph::GEM:
    {
        int16_t w = s * 6 / 16, top = cy - s * 4 / 16, girdle = cy - s / 16, tip = cy + s * 5 / 16;
        tft.fillTriangle(cx - w / 2, top, cx + w / 2, top, cx - w, girdle, body);
        tft.fillTriangle(cx + w / 2, top, cx + w, girdle, cx - w, girdle, body);
        tft.fillTriangle(cx - w, girdle, cx + w, girdle, cx, tip, body);
        tft.drawLine(cx - w / 2, top, cx - w / 3, girdle, hi);
        tft.drawLine(cx - w / 3, girdle, cx, tip, hi);
        tft.drawFastHLine(cx - w + 1, girdle, w / 2, hi);
        break;
    }
    }
}

void TileArt::mine(TFT_eSPI &tft, int16_t x, int16_t y, int16_t s, uint8_t parity, bool blasted, bool lit)
{
    open(tft, x, y, s, parity, 0);
    if (blasted)
    {
        tft.fillRect(x + 1, y + 1, s - 1, s - 1, lit ? (uint16_t)Theme::MINE_HI : (uint16_t)Theme::BLAST);
    }
    mineGlyph(tft, x + s / 2, y + s / 2, s);
}

void TileArt::cursor(TFT_eSPI &tft, int16_t x, int16_t y, int16_t s, bool tight)
{
    uint16_t c = Theme::CURSOR;
    int16_t i = tight ? 1 : 0, l = s / 3;
    int16_t x0 = x + i, y0 = y + i, x1 = x + s - 1 - i, y1 = y + s - 1 - i;
    for (int16_t t = 0; t < 2; t++)
    {
        tft.drawFastHLine(x0, y0 + t, l, c);
        tft.drawFastVLine(x0 + t, y0, l, c);
        tft.drawFastHLine(x1 - l + 1, y0 + t, l, c);
        tft.drawFastVLine(x1 - t, y0, l, c);
        tft.drawFastHLine(x0, y1 - t, l, c);
        tft.drawFastVLine(x0 + t, y1 - l + 1, l, c);
        tft.drawFastHLine(x1 - l + 1, y1 - t, l, c);
        tft.drawFastVLine(x1 - t, y1 - l + 1, l, c);
    }
}

void TileArt::ring(TFT_eSPI &tft, int16_t x, int16_t y, int16_t s, uint8_t dots)
{
    int16_t cx = x + s / 2, cy = y + s / 2, r = s * 5 / 16;
    uint16_t c = Theme::FLAG;
    for (uint8_t i = 0; i < dots && i < RING_DOTS; i++)
    {
        tft.fillRect(cx + RING[i][0] * r / 100 - 1, cy + RING[i][1] * r / 100 - 1, 2, 2, c);
    }
}

void TileArt::sparkle(TFT_eSPI &tft, int16_t x, int16_t y, int16_t s)
{
    int16_t cx = x + s / 2, cy = y + s / 2, r = s / 3;
    uint16_t c = Theme::CURSOR;
    tft.drawFastHLine(cx - r, cy, 2 * r + 1, c);
    tft.drawFastVLine(cx, cy - r, 2 * r + 1, c);
    tft.fillRect(cx - 1, cy - 1, 3, 3, c);
    tft.drawPixel(cx - r / 2, cy - r / 2, c);
    tft.drawPixel(cx + r / 2, cy - r / 2, c);
    tft.drawPixel(cx - r / 2, cy + r / 2, c);
    tft.drawPixel(cx + r / 2, cy + r / 2, c);
}
