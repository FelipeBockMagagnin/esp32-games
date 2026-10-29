#include "Menu.h"

// The idle game's Cave palette, so the launcher looks like one of the family
static const uint16_t BG = 0x0841;
static const uint16_t CARD = 0x20E3;
static const uint16_t TEXT = 0xF75B;
static const uint16_t DIM = 0xBD52;
static const uint16_t MUTED = 0x6B0A;
static const uint16_t CONFIRM = 0xE309;

static const int16_t HEADER_H = 28;
static const int16_t THUMB_SCALE = 2;
static const int16_t THUMB_X = (TFT_WIDTH - THUMB_W * THUMB_SCALE) / 2;
static const int16_t THUMB_Y = 44;
static const int16_t THUMB_BOTTOM = THUMB_Y + THUMB_H * THUMB_SCALE;
static const int16_t NAME_Y = THUMB_BOTTOM + 16;
static const int16_t TAGLINE_Y = NAME_Y + 24;
static const int16_t DOTS_Y = TAGLINE_Y + 22;
static const int16_t HINT_Y = TFT_HEIGHT - 20;
static const int16_t DOT_STEP = 10;

void Menu::centered(const char *text, int16_t y, uint8_t size, uint16_t color)
{
    tft.setTextSize(size);
    tft.setTextDatum(TC_DATUM);
    tft.setTextColor(color, BG);
    tft.drawString(text, TFT_WIDTH / 2, y);
}

void Menu::begin(int start)
{
    selected = start >= 0 && start < catalog.count() ? start : 0;
    drawFrame();
    drawGame();
}

void Menu::drawFrame()
{
    tft.fillScreen(BG);
    tft.fillRect(0, 0, TFT_WIDTH, HEADER_H, CARD);
    tft.setTextSize(2);
    tft.setTextDatum(ML_DATUM);
    tft.setTextColor(TEXT, CARD);
    tft.drawString("dotcompany", 8, HEADER_H / 2);

    if (catalog.count() == 0)
    {
        centered("Nenhum jogo instalado", 130, 1, TEXT);
        centered("Escolha os jogos no site", 150, 1, DIM);
        centered("e grave de novo", 162, 1, DIM);
        return;
    }
    tft.setTextSize(1);
    tft.setTextDatum(ML_DATUM);
    tft.setTextColor(DIM, BG);
    tft.drawString("Setas: escolher", 8, HINT_Y);
    tft.setTextDatum(MR_DATUM);
    tft.setTextColor(CONFIRM, BG);
    tft.drawString("Confirmar: jogar", TFT_WIDTH - 8, HINT_Y);
}

void Menu::drawGame()
{
    if (catalog.count() == 0)
    {
        return;
    }
    const Game &g = catalog.game(selected);
    char counter[8];
    snprintf(counter, sizeof(counter), "%u/%u", selected + 1, catalog.count());
    tft.fillRect(TFT_WIDTH - 48, 0, 48, HEADER_H, CARD);
    tft.setTextSize(1);
    tft.setTextDatum(MR_DATUM);
    tft.setTextColor(DIM, CARD);
    tft.drawString(counter, TFT_WIDTH - 8, HEADER_H / 2);

    drawThumb(THUMB_X, THUMB_Y, g);
    if (catalog.count() > 1)
    {
        tft.setTextSize(2);
        tft.setTextDatum(MC_DATUM);
        tft.setTextColor(MUTED, BG);
        tft.drawString("<", THUMB_X / 2, THUMB_Y + THUMB_H);
        tft.drawString(">", TFT_WIDTH - THUMB_X / 2, THUMB_Y + THUMB_H);
    }

    tft.fillRect(0, NAME_Y, TFT_WIDTH, DOTS_Y + 8 - NAME_Y, BG);
    centered(g.name, NAME_Y, strlen(g.name) > 18 ? 1 : 2, g.accent);
    centered(g.tagline, TAGLINE_Y, 1, DIM);

    int16_t x = TFT_WIDTH / 2 - (catalog.count() - 1) * DOT_STEP / 2;
    for (uint8_t i = 0; i < catalog.count(); i++, x += DOT_STEP)
    {
        if (i == selected)
        {
            tft.fillCircle(x, DOTS_Y + 3, 3, g.accent);
        }
        else
        {
            tft.fillCircle(x, DOTS_Y + 3, 3, BG);
            tft.drawCircle(x, DOTS_Y + 3, 2, MUTED);
        }
    }
}

void Menu::drawThumb(int16_t x, int16_t y, const Game &g)
{
    tft.drawRect(x - 3, y - 3, THUMB_W * THUMB_SCALE + 6, THUMB_H * THUMB_SCALE + 6, g.accent);
    tft.drawRect(x - 2, y - 2, THUMB_W * THUMB_SCALE + 4, THUMB_H * THUMB_SCALE + 4, g.accent);

    static uint16_t pixels[THUMB_W * THUMB_H];
    if (!catalog.readThumb(selected, pixels))
    {
        tft.fillRect(x, y, THUMB_W * THUMB_SCALE, THUMB_H * THUMB_SCALE, CARD);
        char initial[2] = {g.name[0], '\0'};
        tft.setTextSize(6);
        tft.setTextDatum(MC_DATUM);
        tft.setTextColor(g.accent, CARD);
        tft.drawString(initial, x + THUMB_W * THUMB_SCALE / 2, y + THUMB_H * THUMB_SCALE / 2);
        return;
    }
    uint16_t row[THUMB_W * THUMB_SCALE];
    for (int16_t sy = 0; sy < THUMB_H; sy++)
    {
        for (int16_t sx = 0; sx < THUMB_W; sx++)
        {
            row[sx * 2] = row[sx * 2 + 1] = pixels[sy * THUMB_W + sx];
        }
        tft.pushImage(x, y + sy * 2, THUMB_W * THUMB_SCALE, 1, row);
        tft.pushImage(x, y + sy * 2 + 1, THUMB_W * THUMB_SCALE, 1, row);
    }
}

void Menu::step(int delta)
{
    if (catalog.count() < 2)
    {
        return;
    }
    selected = (selected + catalog.count() + delta) % catalog.count();
    drawGame();
}

void Menu::showLaunching()
{
    tft.fillRect(0, HINT_Y - 8, TFT_WIDTH, 16, BG);
    centered("Abrindo...", HINT_Y - 4, 1, catalog.game(selected).accent);
}
