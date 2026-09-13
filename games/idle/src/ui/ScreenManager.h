#ifndef SCREEN_MANAGER_H
#define SCREEN_MANAGER_H

#include <Arduino.h>
#include <TFT_eSPI.h>
#include <vector>
#include "Screen.h"

class ScreenManager
{
private:
    TFT_eSPI &tft;
    std::vector<Screen *> screens;
    int currentIndex;

public:
    ScreenManager(TFT_eSPI &display);

    void addScreen(Screen *screen);
    void setScreen(int index);
    void nextScreen();
    void previousScreen();

    Screen *getCurrentScreen() const;
    int getCurrentIndex() const;
    int getScreenCount() const;

    void update(unsigned long now);
};

#endif // SCREEN_MANAGER_H
