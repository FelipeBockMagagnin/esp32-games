#ifndef DEV_CONSOLE_H
#define DEV_CONSOLE_H

#ifdef DEV_MODE

#include <TFT_eSPI.h>
#include "../game/SaveGame.h"
#include "../managers/ScreenManager.h"

// Serial cheats for testing progression, built only by `pio run -e dev`. Each command edits
// a captured save and loads it back, so the game classes need no setters, and the AutoSaver
// stores the result like any other progress.
class DevConsole
{
public:
    // `changed` runs after every edit, to relock the screens and repaint
    DevConsole(GameState &game, Inventory &inventory, CombatState &combat, Achievements &achievements,
               Unlocks &unlocks, Ascension &ascension, TFT_eSPI &tft, ScreenManager &screens, void (*changed)());

    void update();

private:
    GameState &game;
    Inventory &inventory;
    CombatState &combat;
    Achievements &achievements;
    Unlocks &unlocks;
    Ascension &ascension;
    TFT_eSPI &tft;
    ScreenManager &screens;
    void (*changed)();
    char line[48];
    uint8_t length;
    SaveBlob blob;

    void run(char *command);
    void capture();
    void apply();
    void sendScreen();
};

#endif // DEV_MODE

#endif // DEV_CONSOLE_H
