#include "SplashArt.h"

static const SpriteDef *const FRAMES[SplashArt::FRAME_COUNT] = {&splash_dotcompany_0, &splash_dotcompany_1};
static const SpriteDef &WORDMARK = splash_dotcompany_wordmark;

static const uint8_t CHARACTER_SCALE = 4;
static const uint8_t WORDMARK_SCALE = 2;
// In the brand art the wordmark crosses the character 21 rows below the top of the hood,
// centred on the 5-row band the artist left for it
static const int16_t WORDMARK_TOP = 21 * CHARACTER_SCALE + 2;
static const int16_t MAX_WIDTH = 240;

static uint16_t swapBytes(uint16_t color)
{
    return (uint16_t)((color >> 8) | (color << 8));
}

// Walks a SpriteDef's rows top to bottom, so a row scaled up is decoded from where it starts
// rather than from the top of the runs
struct RowCursor
{
    const SpriteDef &def;
    const uint8_t *runs;
    int16_t row;

    explicit RowCursor(const SpriteDef &def) : def(def), runs(def.runs), row(def.y) {}

    void advanceTo(int16_t target)
    {
        while (row < target && row < def.y + def.h)
        {
            for (uint8_t col = 0; col < def.w; runs += 2)
            {
                col += pgm_read_byte(runs);
            }
            row++;
        }
    }

    void paint(int16_t target, int16_t left, uint8_t scale, uint16_t *line, int16_t width)
    {
        if (target < def.y || target >= def.y + def.h)
        {
            return;
        }
        advanceTo(target);
        const uint8_t *p = runs;
        int16_t px = left + def.x * scale;
        for (uint8_t col = 0; col < def.w;)
        {
            uint8_t length = pgm_read_byte(p++);
            uint8_t index = pgm_read_byte(p++);
            col += length;
            if (index == 0)
            {
                px += length * scale;
                continue;
            }
            uint16_t color = pgm_read_word(def.palette + index - 1);
            for (int16_t k = length * scale; k > 0; k--, px++)
            {
                if (px >= 0 && px < width)
                {
                    line[px] = color;
                }
            }
        }
    }
};

static int16_t canvasWidth(const SpriteDef &def)
{
    return def.x + def.w;
}

static int16_t canvasHeight(const SpriteDef &def)
{
    return def.y + def.h;
}

SplashArt::SplashArt(int16_t x, int16_t y) : UI(x, y), frame(0)
{
    int16_t characterW = 0;
    int16_t characterH = 0;
    for (uint8_t i = 0; i < FRAME_COUNT; i++)
    {
        characterW = max(characterW, canvasWidth(*FRAMES[i]));
        characterH = max(characterH, canvasHeight(*FRAMES[i]));
    }
    characterW *= CHARACTER_SCALE;
    int16_t wordmarkW = canvasWidth(WORDMARK) * WORDMARK_SCALE;

    w = max(characterW, wordmarkW);
    h = max((int16_t)(characterH * CHARACTER_SCALE), (int16_t)(WORDMARK_TOP + canvasHeight(WORDMARK) * WORDMARK_SCALE));
    characterLeft = (w - characterW) / 2;
    wordmarkLeft = (w - wordmarkW) / 2;
}

void SplashArt::draw(TFT_eSPI &tft)
{
    // Palette words are byte-swapped like every other asset, so the fill colour is too
    uint16_t fill = swapBytes(eraseColor);
    uint16_t line[MAX_WIDTH];
    int16_t width = min(w, MAX_WIDTH);
    RowCursor character(*FRAMES[frame]);
    RowCursor wordmark(WORDMARK);

    tft.startWrite();
    tft.setAddrWindow(x, y, width, h);
    for (int16_t row = 0; row < h; row++)
    {
        for (int16_t i = 0; i < width; i++)
        {
            line[i] = fill;
        }
        character.paint(row / CHARACTER_SCALE, characterLeft, CHARACTER_SCALE, line, width);
        if (row >= WORDMARK_TOP)
        {
            wordmark.paint((row - WORDMARK_TOP) / WORDMARK_SCALE, wordmarkLeft, WORDMARK_SCALE, line, width);
        }
        tft.pushPixels(line, width);
    }
    tft.endWrite();
}
