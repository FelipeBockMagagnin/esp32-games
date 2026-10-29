#include "ConfirmScreen.h"
#include "../gfx/Widgets.h"

void ConfirmScreen::onEnter(TFT_eSPI &tft)
{
    yes = false;
    Screen::onEnter(tft);
}

void ConfirmScreen::onUpPress()
{
    yes = !yes;
    game.sound.playMenu();
    forceFrame();
}

void ConfirmScreen::onDownPress()
{
    onUpPress();
}

bool ConfirmScreen::onBackPress()
{
    game.screens.closeModal();
    return true;
}

void ConfirmScreen::onConfirmPress()
{
    if (!yes)
    {
        game.sound.playMenu();
        game.screens.closeModal();
        return;
    }
    onYes();
}

void ConfirmScreen::drawScene(Canvas &c) const
{
    Widgets::card(c, 16, 70, 208, 110, 20);
    c.text(120, 86, text.question, FONT_TITLE, Theme::rgb(INK), Canvas::CENTER);
    c.text(120, 120, text.line1, FONT_BODY, Theme::rgb(INK_SOFT), Canvas::CENTER);
    c.text(120, 138, text.line2, FONT_BODY, Theme::rgb(INK_SOFT), Canvas::CENTER);
    const char *options[2] = {"No, keep it", text.yes};
    for (uint8_t i = 0; i < 2; i++)
    {
        float y = 200 + i * 44;
        bool on = (i == 1) == yes;
        uint8_t fill = on ? (i == 1 ? accent : ACCENT) : CARD;
        c.fillRoundRect(30, y + 2, 180, 34, 17, Theme::rgb(SHADOW), 90);
        c.fillRoundRect(30, y, 180, 34, 17, Theme::rgb(fill));
        c.text(120, y + 9, options[i], FONT_BODY, Theme::rgb(on ? ACCENT_INK : i == 1 ? accent : INK), Canvas::CENTER);
    }
    Widgets::hint(c, 304, "up/down choose  -  left: back");
}
