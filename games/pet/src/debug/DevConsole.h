#ifndef DEV_CONSOLE_H
#define DEV_CONSOLE_H

#ifdef DEV_MODE

#include "../game/SaveGame.h"

// Serial cheats for testing progression, built only by `pio run -e dev`. Each command edits
// a captured save and loads it back, so the game classes need no setters, and the AutoSaver
// stores the result like any other progress.
class DevConsole
{
public:
    // `changed` runs after every edit, to repaint
    DevConsole(World &world, Colony &colony, void (*changed)());

    void update();

private:
    World &world;
    Colony &colony;
    void (*changed)();
    char line[48];
    uint8_t length;
    SaveBlob blob;

    void run(char *command);
};

#endif // DEV_MODE

#endif // DEV_CONSOLE_H
