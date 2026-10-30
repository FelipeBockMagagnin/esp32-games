#include "LcdMatrix.h"
#include "Theme.h"

// Five rows of three bits, top row first
static const uint16_t LETTERS[26] = {
    0b010101111101101, 0b110101110101110, 0b011100100100011, 0b110101101101110, 0b111100110100111,
    0b111100110100100, 0b011100101101011, 0b101101111101101, 0b111010010010111, 0b001001001101010,
    0b101101110101101, 0b100100100100111, 0b101111111101101, 0b110101101101101, 0b010101101101010,
    0b110101110100100, 0b010101101111011, 0b110101110101101, 0b011100010001110, 0b111010010010010,
    0b101101101101111, 0b101101101101010, 0b101101111111101, 0b101101010101101, 0b101101010010010,
    0b111001010100111};
static const uint16_t DIGITS[10] = {0b111101101101111, 0b010110010010111, 0b110001010100111, 0b110001010001110,
                                    0b101101111001001, 0b111100110001110, 0b011100111101111, 0b111001010010010,
                                    0b111101111101111, 0b111101111001110};

static uint16_t glyph(char c)
{
    if (c >= 'A' && c <= 'Z')
    {
        return LETTERS[c - 'A'];
    }
    if (c >= '0' && c <= '9')
    {
        return DIGITS[c - '0'];
    }
    switch (c)
    {
    case ':':
        return 0b000010000010000;
    case '-':
        return 0b000000111000000;
    case '/':
        return 0b001001010100100;
    case '?':
        return 0b110001010000010;
    case '!':
        return 0b010010010000010;
    case '>':
        return 0b100010001010100;
    default:
        return 0;
    }
}

void LcdMatrix::set(int16_t x, int16_t y, bool on)
{
    if (x < 0 || y < 0 || x >= COLS || y >= ROWS)
    {
        return;
    }
    if (on)
    {
        rows[y] |= 1u << x;
    }
    else
    {
        rows[y] &= ~(1u << x);
    }
}

void LcdMatrix::fill(int16_t x, int16_t y, int16_t w, int16_t h, bool on)
{
    for (int16_t j = 0; j < h; j++)
    {
        for (int16_t i = 0; i < w; i++)
        {
            set(x + i, y + j, on);
        }
    }
}

void LcdMatrix::sprite(const Sprite &art, int16_t x, int16_t y, bool mirror, Pose pose)
{
    for (int16_t j = 0; j < art.h; j++)
    {
        for (int16_t i = 0; i < art.w; i++)
        {
            if (spriteDot(art.art[j * art.w + i], pose))
            {
                set(mirror ? x + art.w - 1 - i : x + i, y + j);
            }
        }
    }
}

void LcdMatrix::spriteOn(const Sprite &art, int16_t x, int16_t bottom, bool mirror, Pose pose)
{
    sprite(art, x, bottom - art.h, mirror, pose);
}

void LcdMatrix::text(int16_t x, int16_t y, const char *s)
{
    for (; *s != '\0'; s++, x += CHAR_W)
    {
        uint16_t g = glyph(*s);
        for (int16_t j = 0; j < 5; j++)
        {
            for (int16_t i = 0; i < 3; i++)
            {
                if (g >> (14 - j * 3 - i) & 1)
                {
                    set(x + i, y + j);
                }
            }
        }
    }
}

void LcdMatrix::textCentered(int16_t y, const char *s)
{
    text((COLS - ((int16_t)strlen(s) * CHAR_W - 1)) / 2, y, s);
}

void LcdMatrix::present(TFT_eSPI &tft)
{
    uint32_t flip = inverted ? 0xFFFFFFFFu : 0;
    uint16_t lit = Theme::INK;
    uint16_t unlit = Theme::GHOST;
    for (int16_t y = 0; y < ROWS; y++)
    {
        uint32_t now = rows[y] ^ flip;
        uint32_t changed = stale ? 0xFFFFFFFFu : now ^ shown[y];
        shown[y] = now;
        for (int16_t x = 0; changed != 0; x++, changed >>= 1)
        {
            if (changed & 1)
            {
                tft.fillRect(X + x * DOT, Y + y * DOT, DOT - 1, DOT - 1, now >> x & 1 ? lit : unlit);
            }
        }
    }
    stale = false;
}
