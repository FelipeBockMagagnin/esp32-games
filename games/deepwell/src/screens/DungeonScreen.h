#ifndef DUNGEON_SCREEN_H
#define DUNGEON_SCREEN_H

#include "Screen.h"
#include "../ui/Text.h"
#include "../ui/ProgressBar.h"
#include "../ui/TileMap.h"
#include "../rogue/Run.h"
#include "../core/ScreenManager.h"
#include "../core/SoundManager.h"

// The game: a HUD over the map. Every direction walks (or attacks what is in the way), held
// down it keeps walking until something happens, and Confirm opens the action menu.
class DungeonScreen : public Screen
{
public:
    static const int16_t HUD_H = 32;

    // `floorReached` runs after the stairs and `died` after a death, for the save
    DungeonScreen(Run &run, SoundManager &sound, ScreenManager &screenManager, Screen &menu,
                  void (*floorReached)(), void (*died)());

    // The one way into the run, also for the menu's actions, so every turn gets its sound
    void act(Action action);

    void onEnter(TFT_eSPI &tft) override;
    void update(unsigned long now) override;

    void onUpPress() override { act(Action::NORTH); }
    void onDownPress() override { act(Action::SOUTH); }
    bool onBackPress() override
    {
        act(Action::WEST);
        return true;
    }
    bool onRightPress() override
    {
        act(Action::EAST);
        return true;
    }
    void onConfirmPress() override;
    bool onRepeat(Key key) override;

private:
    Text statsText;
    ProgressBar hpBar;
    Text logLines[Run::MESSAGE_LINES];
    TileMap tileMap;

    Run &run;
    SoundManager &sound;
    ScreenManager &screenManager;
    Screen &menu;
    void (*floorReached)();
    void (*died)();
    uint16_t seenRevision;
    uint16_t lastEvents;

    void refresh();
};

#endif // DUNGEON_SCREEN_H
