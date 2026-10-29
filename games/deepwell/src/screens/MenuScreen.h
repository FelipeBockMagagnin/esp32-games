#ifndef MENU_SCREEN_H
#define MENU_SCREEN_H

#include "Screen.h"
#include "DungeonScreen.h"
#include "../ui/Header.h"
#include "ListRow.h"
#include "../ui/Text.h"

// The actions that are not a step (descend, drink, wait, settings) above a character sheet.
// A modal over the dungeon; Left closes it.
class MenuScreen : public Screen
{
public:
    MenuScreen(const Run &run, ScreenManager &screenManager, SoundManager &sound, Screen &settingsScreen);

    // Set after construction: the dungeon screen holds this menu too
    void setDungeon(DungeonScreen *screen) { dungeon = screen; }

    void onEnter(TFT_eSPI &tft) override;
    void onUpPress() override;
    void onDownPress() override;
    void onConfirmPress() override;
    bool onBackPress() override;

private:
    enum Option : uint8_t
    {
        OPTION_DESCEND,
        OPTION_DRINK,
        OPTION_WAIT,
        OPTION_SETTINGS,
        OPTION_COUNT
    };
    static const uint8_t SHEET_LINES = 5;

    Header header;
    ListRow rows[OPTION_COUNT];
    Text sheet[SHEET_LINES];
    Text hintText;

    const Run &run;
    ScreenManager &screenManager;
    SoundManager &sound;
    Screen &settingsScreen;
    DungeonScreen *dungeon;
    Option options[OPTION_COUNT]; // What each visible row does; Descend only shows on the stairs
    uint8_t optionCount;
    uint8_t selected;

    void build();
    void refreshRows();
};

#endif // MENU_SCREEN_H
