#ifndef ALBUM_SCREEN_H
#define ALBUM_SCREEN_H

#include "Screen.h"
#include "Game.h"

// Every form there is, met or not yet, and the pets that passed an egg on
class AlbumScreen : public Screen
{
public:
    AlbumScreen(Display &display, Game &game) : Screen(display), game(game) {}

    void onEnter(TFT_eSPI &tft) override;
    void onUpPress() override;
    void onDownPress() override;
    bool onRepeat(Key key) override;

protected:
    const char *title() const override { return "Album"; }
    void drawScene(Canvas &c) const override;

private:
    Game &game;
    uint8_t selected = 0;
    unsigned long selectedAt = 0;
    void move(int8_t dir);
};

#endif // ALBUM_SCREEN_H
