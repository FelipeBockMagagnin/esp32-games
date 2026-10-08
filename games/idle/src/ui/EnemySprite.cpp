#include "EnemySprite.h"
#include "Theme.h"

static const int16_t CARD_W = 76;
static const int16_t CARD_H = 40;

static uint16_t swapBytes(uint16_t color)
{
    return (uint16_t)((color >> 8) | (color << 8));
}

// Paints the opaque pixels of one frame row of `def` into `line`, scaled, leaving the
// transparent ones as they were. `runs` walks the encoded rows in order, so it must be
// called for every frame row, top to bottom.
static void paintRow(const SpriteDef &def, const uint8_t *&runs, uint8_t row, uint16_t *line)
{
    if (row < def.y || row >= def.y + def.h)
    {
        return;
    }

    // Runs never cross a row, so this row's runs add up to exactly w pixels
    int16_t px = def.x * EnemySprite::SCALE;
    for (uint8_t col = 0; col < def.w;)
    {
        uint8_t length = pgm_read_byte(runs++);
        uint8_t index = pgm_read_byte(runs++);
        col += length;
        if (index == 0)
        {
            px += length * EnemySprite::SCALE;
            continue;
        }
        uint16_t color = pgm_read_word(def.palette + index - 1);
        for (int16_t k = length * EnemySprite::SCALE; k > 0; k--)
        {
            line[px++] = color;
        }
    }
}

void EnemySprite::draw(TFT_eSPI &tft)
{
    if (sprite == nullptr && background == nullptr)
    {
        tft.fillRect(x, y, w, h, eraseColor);
        drawCountdown(tft);
        return;
    }

    // Palette words are byte-swapped like every other asset, so the fill colour is too
    uint16_t fill = swapBytes(eraseColor);
    uint16_t line[SIZE];
    const uint8_t *spriteRuns = sprite != nullptr ? sprite->runs : nullptr;
    const uint8_t *backgroundRuns = background != nullptr ? background->runs : nullptr;

    tft.startWrite();
    tft.setAddrWindow(x, y, SIZE, SIZE);
    for (uint8_t row = 0; row < SPRITE_FRAME; row++)
    {
        for (int16_t i = 0; i < SIZE; i++)
        {
            line[i] = fill;
        }
        if (background != nullptr)
        {
            paintRow(*background, backgroundRuns, row, line);
        }
        if (sprite != nullptr)
        {
            paintRow(*sprite, spriteRuns, row, line);
        }

        for (uint8_t k = 0; k < SCALE; k++)
        {
            tft.pushPixels(line, SIZE);
        }
    }
    tft.endWrite();
    drawCountdown(tft);
}

void EnemySprite::drawCountdown(TFT_eSPI &tft)
{
    if (countdown == 0)
    {
        return;
    }

    int16_t cx = x + w / 2;
    int16_t top = y + (h - CARD_H) / 2;
    tft.fillRoundRect(cx - CARD_W / 2, top, CARD_W, CARD_H, 4, Theme::CARD);
    tft.drawRoundRect(cx - CARD_W / 2, top, CARD_W, CARD_H, 4, Theme::FRAME);

    char buf[6];
    snprintf(buf, sizeof(buf), "%us", (unsigned)countdown);
    tft.setTextDatum(TC_DATUM);
    tft.setTextSize(1);
    tft.setTextColor(Theme::TEXT_2);
    tft.drawString("NEXT ENEMY", cx, top + 6);
    tft.setTextSize(2);
    tft.setTextColor(Theme::TEXT);
    tft.drawString(buf, cx, top + 19);
}
