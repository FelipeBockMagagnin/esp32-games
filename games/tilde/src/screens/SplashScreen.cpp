#include "SplashScreen.h"
#include <math.h>
#include "../art/IdlePose.h"
#include "../gfx/Widgets.h"

void SplashScreen::onEnter(TFT_eSPI &tft)
{
    shownAt = millis();
    game.sound.playStart();
    Screen::onEnter(tft);
}

void SplashScreen::update(unsigned long now)
{
    if (now - shownAt >= SHOW_MS)
    {
        game.screens.closeModal();
    }
}

void SplashScreen::drawScene(Canvas &c) const
{
    unsigned long now = frameTime(), t = now - shownAt;
    c.fillCircle(120, 150, 78, Theme::rgb(WALL_TOP));
    c.fillEllipse(120, 196, 52, 8, Theme::rgb(RUG), 200);
    Pose p = idlePose(game.pet, now, 120, 194, 1.25f);
    // It drops in and lands with a squash
    float fall = t < 500 ? 1 - t / 500.0f : 0;
    p.lift = fall * fall * 120;
    if (t >= 500 && t < 900)
    {
        float u = (t - 500) / 400.0f;
        p.squashY = 1 - 0.2f * expf(-u * 4) * cosf(u * 12);
        p.squashX = 2 - p.squashY;
    }
    p.eye = t > 900 ? Eye::HAPPY : Eye::OPEN;
    p.mouth = Mouth::W;
    p.blush = 1;
    drawPet(c, game.pet.getForm(), p);
    float u = fminf(1, t / 600.0f);
    c.text(120, 236, "tilde", FONT_TITLE, Theme::rgb(INK), Canvas::CENTER, (uint8_t)(u * 255));
    c.text(120, 262, "a little friend to care for", FONT_SMALL, Theme::rgb(INK_SOFT), Canvas::CENTER,
           (uint8_t)(u * 255));
    for (uint8_t i = 0; i < 3; i++)
    {
        float bounce = fabsf(sinf(now / 200.0f - i * 0.6f)) * 4;
        c.fillCircle(108 + i * 12, 292 - bounce, 3.5f, Theme::rgb(i * 800 < t ? ACCENT : CARD_EDGE));
    }
}
