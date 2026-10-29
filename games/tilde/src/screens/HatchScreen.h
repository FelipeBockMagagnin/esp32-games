#ifndef HATCH_SCREEN_H
#define HATCH_SCREEN_H

#include "Screen.h"
#include "Game.h"

// A modal once the egg hatches: the baby says hello and the player names it, a letter at a
// time (Up/Down pick the letter, Left/Right move along, Confirm keeps the name)
class HatchScreen : public Screen
{
public:
    HatchScreen(Display &display, Game &game) : Screen(display), game(game) {}

    void onEnter(TFT_eSPI &tft) override;
    void onUpPress() override;
    void onDownPress() override;
    void onConfirmPress() override;
    bool onBackPress() override;
    bool onRightPress() override;
    bool onRepeat(Key key) override;
    bool allowsGestures() const override { return false; }

protected:
    const char *title() const override { return "Hello!"; }
    void drawScene(Canvas &c) const override;

private:
    Game &game;
    char name[NAME_LEN + 1];
    uint8_t cursor = 0;
    unsigned long movedAt = 0;

    void step(int8_t dir);
};

#endif // HATCH_SCREEN_H
