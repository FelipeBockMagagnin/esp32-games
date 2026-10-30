#include "SplashScreen.h"
#include "../ui/Themes.h"

static const unsigned long SPLASH_MS = 2600;
static const unsigned long STEP_MS = 60;
static const int16_t RUNNER_W = 120;
static const int16_t RUNNER_H = 12;
static const int16_t RUNNER_X = (DISPLAY_W - RUNNER_W) / 2;
static const int16_t RUNNER_Y = 230;
static const int16_t BLOCK = 8;

// The boot screen is black on every theme, as the real ones were, with the flag's colours
static const uint16_t BLACK = 0x0000;
static const uint16_t FLAG[4] = {rgb(0xf35325), rgb(0x81bc06), rgb(0x05a6f0), rgb(0xffba08)};
static const uint16_t GREY = rgb(0x8a8a8a);

void SplashScreen::Logo::draw(TFT_eSPI &tft)
{
    tft.fillScreen(BLACK);
    // A waving flag: each quarter is a column of rows bent by a little sine-like offset
    static const int8_t WAVE[] = {0, 1, 2, 2, 3, 3, 3, 2, 2, 1, 0, -1, -1, -2, -2, -2, -1, -1, 0, 0};
    const int16_t size = 26;
    const int16_t left = DISPLAY_W / 2 - size - 3;
    const int16_t top = 90;
    for (uint8_t q = 0; q < 4; q++)
    {
        int16_t qx = left + (q % 2) * (size + 6);
        int16_t qy = top + (q / 2) * (size + 6);
        for (int16_t col = 0; col < size; col++)
        {
            int16_t offset = WAVE[(col + (q % 2) * size) * 20 / (2 * size + 6)];
            tft.drawFastVLine(qx + col, qy + offset, size, FLAG[q]);
        }
    }
    tft.setTextDatum(MC_DATUM);
    tft.setTextSize(3);
    tft.setTextColor(rgb(0xffffff));
    tft.drawString("NaN", DISPLAY_W / 2 - 41, 180);
    tft.setTextColor(FLAG[0]);
    tft.drawString("idle", DISPLAY_W / 2 + 32, 180);
    tft.setTextSize(1);
    tft.setTextColor(GREY);
    tft.drawString("Edicao dotcompany", DISPLAY_W / 2, 204);
    tft.drawString("Iniciando...", DISPLAY_W / 2, 290);
    tft.setTextDatum(TL_DATUM);
}

SplashScreen::Runner::Runner() : UI(RUNNER_X, RUNNER_Y, RUNNER_W, RUNNER_H)
{
}

void SplashScreen::Runner::draw(TFT_eSPI &tft)
{
    tft.drawRoundRect(x, y, w, h, 3, GREY);
    tft.fillRect(x + 2, y + 2, w - 4, h - 4, BLACK);
    int16_t span = w - 4 + 3 * (BLOCK + 2);
    int16_t lead = (step * 4) % span;
    for (uint8_t b = 0; b < 3; b++)
    {
        int16_t bx = x + 2 + lead - b * (BLOCK + 2) - BLOCK;
        for (int16_t i = 0; i < BLOCK; i++)
        {
            int16_t px = bx + i;
            if (px >= x + 2 && px < x + w - 2)
            {
                tft.drawFastVLine(px, y + 3, h - 6, FLAG[2]);
            }
        }
    }
}

SplashScreen::SplashScreen(ScreenManager &screenManager, SoundManager &sound)
    : Screen(nullptr, Color(BLACK)), screenManager(screenManager), sound(sound)
{
    addElement(&logo);
    addElement(&runner);
}

void SplashScreen::onEnter(TFT_eSPI &tft)
{
    started = false;
    Screen::onEnter(tft);
}

void SplashScreen::update(unsigned long now)
{
    if (!started)
    {
        started = true;
        start = now;
        sound.playStartup();
    }
    runner.setStep((uint8_t)((now - start) / STEP_MS));
    if (now - start >= SPLASH_MS)
    {
        finish();
    }
}

void SplashScreen::finish()
{
    screenManager.closeModal();
}
