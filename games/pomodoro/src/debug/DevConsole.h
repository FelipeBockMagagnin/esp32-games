#ifndef DEV_CONSOLE_H
#define DEV_CONSOLE_H

#ifdef DEV_MODE

#include <TFT_eSPI.h>
#include "../app/FocusTimer.h"
#include "../managers/ScreenManager.h"

// Serial commands for testing, built only by `pio run -e dev`: `end` runs the current phase
// out, `screen <i>` switches screen, `shot` sends the display to tools/screenshot.py
class DevConsole
{
public:
    DevConsole(FocusTimer &timer, TFT_eSPI &tft, ScreenManager &screens);

    void update();

private:
    FocusTimer &timer;
    TFT_eSPI &tft;
    ScreenManager &screens;
    char line[32];
    uint8_t length;

    void run(char *command);
    void sendScreen();
};

#endif // DEV_MODE

#endif // DEV_CONSOLE_H
