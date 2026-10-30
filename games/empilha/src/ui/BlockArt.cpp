#include "BlockArt.h"
#include "Themes.h"

static const uint16_t WHITE = 0xFFFF;
static const uint16_t BLACK = 0x0000;

// `a` over `b`, in eighths
static uint16_t mix(uint16_t a, uint16_t b, uint8_t eighths)
{
    uint8_t rest = 8 - eighths;
    uint16_t r = ((a >> 11) * eighths + (b >> 11) * rest) / 8;
    uint16_t g = (((a >> 5) & 0x3F) * eighths + ((b >> 5) & 0x3F) * rest) / 8;
    uint16_t bl = ((a & 0x1F) * eighths + (b & 0x1F) * rest) / 8;
    return (uint16_t)(r << 11 | g << 5 | bl);
}

static void bevel(TFT_eSPI &tft, int16_t x, int16_t y, int16_t s, uint16_t c)
{
    uint16_t hi = mix(WHITE, c, 3), lo = mix(BLACK, c, 3);
    tft.fillRect(x, y, s, s, c);
    tft.drawFastHLine(x, y, s, hi);
    tft.drawFastVLine(x, y, s, hi);
    tft.drawFastHLine(x, y + s - 1, s, lo);
    tft.drawFastVLine(x + s - 1, y, s, lo);
    if (s >= 12)
    {
        tft.fillRect(x + 3, y + 3, 3, 2, hi);
    }
}

static void crate(TFT_eSPI &tft, int16_t x, int16_t y, int16_t s, uint16_t c)
{
    uint16_t lo = mix(BLACK, c, 3), hi = mix(WHITE, c, 2);
    tft.fillRect(x, y, s, s, c);
    tft.drawLine(x + 1, y + s - 2, x + s - 2, y + 1, lo);
    tft.drawFastHLine(x + 1, y + 1, s - 2, hi);
    tft.drawRect(x, y, s, s, lo);
    if (s >= 12)
    {
        tft.drawFastHLine(x + 1, y + s / 3, s - 2, mix(BLACK, c, 1));
        tft.drawFastHLine(x + 1, y + 2 * s / 3, s - 2, mix(BLACK, c, 1));
        tft.drawLine(x + 1, y + s - 2, x + s - 2, y + 1, lo);
        tft.drawPixel(x + 2, y + 2, lo);
        tft.drawPixel(x + s - 3, y + 2, lo);
        tft.drawPixel(x + 2, y + s - 3, lo);
        tft.drawPixel(x + s - 3, y + s - 3, lo);
    }
}

static void sketch(TFT_eSPI &tft, int16_t x, int16_t y, int16_t s, uint16_t c, uint16_t ground)
{
    tft.fillRect(x, y, s, s, ground);
    uint16_t pencil = mix(c, ground, 5);
    int16_t last = s - 2;
    for (int16_t d = 3; d <= 2 * last; d += 3)
    {
        int16_t x0 = d < last ? d : last, y0 = d < last ? 1 : d - last + 1;
        int16_t x1 = d < last ? 1 : d - last + 1, y1 = d < last ? d : last;
        tft.drawLine(x + x0, y + y0, x + x1, y + y1, pencil);
    }
    tft.drawRect(x, y, s, s, c);
}

static void glow(TFT_eSPI &tft, int16_t x, int16_t y, int16_t s, uint16_t c, uint16_t ground)
{
    tft.fillRect(x, y, s, s, ground);
    tft.fillRect(x + 1, y + 1, s - 2, s - 2, mix(c, BLACK, 2));
    tft.drawRect(x + 1, y + 1, s - 2, s - 2, c);
    if (s >= 12)
    {
        tft.drawRect(x + 2, y + 2, s - 4, s - 4, mix(c, BLACK, 4));
    }
}

static void jelly(TFT_eSPI &tft, int16_t x, int16_t y, int16_t s, uint16_t c, uint16_t ground)
{
    tft.fillRect(x, y, s, s, ground);
    tft.fillRoundRect(x + 1, y + 1, s - 2, s - 2, s / 4, c);
    tft.drawFastHLine(x + s / 4 + 1, y + 2, s / 2 - 1, mix(WHITE, c, 6));
    tft.drawFastHLine(x + s / 4 + 1, y + s - 3, s / 2 - 1, mix(BLACK, c, 2));
}

static void lcd(TFT_eSPI &tft, int16_t x, int16_t y, int16_t s, uint16_t c, uint16_t ground)
{
    tft.fillRect(x, y, s, s, ground);
    tft.drawRect(x + 1, y + 1, s - 2, s - 2, c);
    tft.fillRect(x + 3, y + 3, s - 6, s - 6, c);
}

void BlockArt::block(TFT_eSPI &tft, int16_t x, int16_t y, int16_t s, uint8_t type, uint16_t ground)
{
    uint16_t c = Theme::piece(type);
    switch (activeTheme().style)
    {
    case BlockStyle::BEVEL:
        bevel(tft, x, y, s, c);
        break;
    case BlockStyle::CRATE:
        crate(tft, x, y, s, c);
        break;
    case BlockStyle::SKETCH:
        sketch(tft, x, y, s, c, ground);
        break;
    case BlockStyle::GLOW:
        glow(tft, x, y, s, c, ground);
        break;
    case BlockStyle::JELLY:
        jelly(tft, x, y, s, c, ground);
        break;
    case BlockStyle::LCD:
        lcd(tft, x, y, s, c, ground);
        break;
    }
}

void BlockArt::empty(TFT_eSPI &tft, int16_t x, int16_t y, int16_t s)
{
    tft.fillRect(x, y, s, s, Theme::WELL);
    tft.drawFastHLine(x, y + s - 1, s, Theme::GRID);
    tft.drawFastVLine(x + s - 1, y, s, Theme::GRID);
}

void BlockArt::ghost(TFT_eSPI &tft, int16_t x, int16_t y, int16_t s, uint8_t type)
{
    empty(tft, x, y, s);
    uint16_t edge = mix(Theme::piece(type), Theme::WELL, 4);
    tft.drawRect(x + 1, y + 1, s - 3, s - 3, edge);
}

void BlockArt::flash(TFT_eSPI &tft, int16_t x, int16_t y, int16_t s)
{
    tft.fillRect(x, y, s, s, Theme::FLASH);
}

void BlockArt::dead(TFT_eSPI &tft, int16_t x, int16_t y, int16_t s)
{
    uint16_t c = Theme::DEAD;
    tft.fillRect(x, y, s, s, c);
    tft.drawRect(x, y, s, s, mix(BLACK, c, 2));
}
