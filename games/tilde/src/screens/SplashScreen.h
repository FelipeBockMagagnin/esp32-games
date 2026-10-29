#ifndef SPLASH_SCREEN_H
#define SPLASH_SCREEN_H

#include "Screen.h"
#include "Game.h"

// The title over a boot, gone after a moment or at any press
class SplashScreen : public Screen
{
public:
    SplashScreen(Display &display, Game &game) : Screen(display), game(game) {}

    void onEnter(TFT_eSPI &tft) override;
    void update(unsigned long now) override;
    void onConfirmPress() override { game.screens.closeModal(); }
    void onUpPress() override { game.screens.closeModal(); }
    void onDownPress() override { game.screens.closeModal(); }
    bool onBackPress() override
    {
        game.screens.closeModal();
        return true;
    }
    bool onRightPress() override
    {
        game.screens.closeModal();
        return true;
    }
    bool allowsGestures() const override { return false; }

protected:
    void drawScene(Canvas &c) const override;

private:
    static const unsigned long SHOW_MS = 2400;
    Game &game;
    unsigned long shownAt = 0;
};

#endif // SPLASH_SCREEN_H
