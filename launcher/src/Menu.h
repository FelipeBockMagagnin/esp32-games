#ifndef MENU_H
#define MENU_H

#include <TFT_eSPI.h>
#include "Catalog.h"

// One game at a time: its icon at twice its size, name, tagline and a dot per game
class Menu
{
public:
    Menu(TFT_eSPI &tft, const Catalog &catalog) : tft(tft), catalog(catalog) {}

    void begin(int selected);
    void step(int delta);
    void showLaunching();
    uint8_t getSelected() const { return selected; }

private:
    void drawFrame();
    void drawGame();
    void drawThumb(int16_t x, int16_t y, const Game &g);
    void centered(const char *text, int16_t y, uint8_t size, uint16_t color);

    TFT_eSPI &tft;
    const Catalog &catalog;
    uint8_t selected = 0;
};

#endif // MENU_H
