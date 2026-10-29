#ifndef STATS_SCREEN_H
#define STATS_SCREEN_H

#include "Screen.h"
#include "Game.h"

// The pet in numbers: its portrait, age and growth, every need as a bar, the bond and the mood
class StatsScreen : public Screen
{
public:
    StatsScreen(Display &display, Game &game) : Screen(display), game(game) {}

protected:
    const char *title() const override { return "Stats"; }
    void drawScene(Canvas &c) const override;

private:
    Game &game;
};

#endif // STATS_SCREEN_H
