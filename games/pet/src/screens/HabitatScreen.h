#ifndef HABITAT_SCREEN_H
#define HABITAT_SCREEN_H

#include "Screen.h"
#include "../ui/Text.h"
#include "Box.h"
#include "../game/World.h"
#include "../game/Colony.h"
#include "../managers/SoundManager.h"
#include "habitat/TileGrid.h"
#include "BuildMenuScreen.h"

// The map. Every arrow moves the cursor; Left and Right leave the screen only from the edge
// of the grid. Confirm does the obvious thing to the tile (clear, water, harvest) or opens
// the build menu.
class HabitatScreen : public Screen
{
public:
    HabitatScreen(World &world, Colony &colony, SoundManager &sound, BuildMenuScreen &menu);

    void update(unsigned long now) override;

    void onConfirmPress() override;
    void onUpPress() override { move(0, -1); }
    void onDownPress() override { move(0, 1); }
    bool onBackPress() override { return move(-1, 0); }
    bool onNextPress() override { return move(1, 0); }
    bool repeatsArrows() const override { return true; }
    bool onArrowRepeat(Arrow arrow) override;

private:
    Header header;
    TileGrid grid;
    Box divider;
    Text tileText;
    Text hintText;
    Text coinsText;
    Text foodText;
    Text popText;

    World &world;
    Colony &colony;
    SoundManager &sound;
    BuildMenuScreen &menu;
    uint8_t cursorX;
    uint8_t cursorY;

    bool move(int8_t dx, int8_t dy);
    void refreshStatus();
    void describeTile(char *line, size_t lineSize, char *hint, size_t hintSize, Color &color);
};

#endif // HABITAT_SCREEN_H
