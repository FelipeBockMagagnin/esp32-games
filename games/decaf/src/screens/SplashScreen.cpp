#include "SplashScreen.h"

// The logo's own backdrop, brand art like the level palettes rather than a theme colour
static constexpr Color BACKDROP = Color(0x5AEB);

static const unsigned long SHOW_MS = 2500;
static const unsigned long FRAME_MS = 500; // The animated logo's frame time

SplashScreen::SplashScreen(ScreenManager &screenManager, SoundManager &sound)
    : Screen(BACKDROP), art(0, 0), screenManager(screenManager), sound(sound), start(0)
{
    art.x = (DISPLAY_W - art.w) / 2;
    art.y = (DISPLAY_H - art.h) / 2;
    addElement(&art);
}

void SplashScreen::onEnter(TFT_eSPI &tft)
{
    start = 0;
    art.setFrame(0);
    sound.playJingle();
    Screen::onEnter(tft);
}

void SplashScreen::update(unsigned long now)
{
    if (start == 0)
    {
        start = now;
    }
    unsigned long shown = now - start;
    if (shown >= SHOW_MS)
    {
        close();
        return;
    }
    art.setFrame((shown / FRAME_MS) % SplashArt::FRAME_COUNT);
}

void SplashScreen::close()
{
    screenManager.closeModal();
}
