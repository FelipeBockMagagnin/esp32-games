#ifndef CONFIRM_SCREEN_H
#define CONFIRM_SCREEN_H

#include "Screen.h"
#include "Game.h"

// A yes-or-no modal; No is selected first, so a stray Confirm never does it
class ConfirmScreen : public Screen
{
public:
    struct Text
    {
        const char *title;
        const char *question;
        const char *line1;
        const char *line2;
        const char *yes;
    };

    ConfirmScreen(Display &display, Game &game, const Text &text, uint8_t accent, void (*onYes)())
        : Screen(display), game(game), text(text), accent(accent), onYes(onYes)
    {
    }

    void onEnter(TFT_eSPI &tft) override;
    void onUpPress() override;
    void onDownPress() override;
    void onConfirmPress() override;
    bool onBackPress() override;

protected:
    const char *title() const override { return text.title; }
    void drawScene(Canvas &c) const override;

private:
    Game &game;
    Text text;
    uint8_t accent;
    void (*onYes)();
    bool yes = false;
};

#endif // CONFIRM_SCREEN_H
