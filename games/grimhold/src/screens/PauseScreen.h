#ifndef PAUSE_SCREEN_H
#define PAUSE_SCREEN_H

#include "Screen.h"
#include "Header.h"
#include "ListRow.h"
#include "Text.h"
#include "../core/ScreenManager.h"
#include "../core/SoundManager.h"
#include "../crawl/Run.h"

// A modal over a paused run: back to it, the inventory, the potions, Settings, giving up
class PauseScreen : public Screen
{
public:
    PauseScreen(Run &run, ScreenManager &screenManager, SoundManager &sound, Screen &playScreen,
                Screen &inventoryScreen, Screen &settingsScreen, Screen &abandonScreen);

    void onEnter(TFT_eSPI &tft) override;
    void onUpPress() override;
    void onDownPress() override;
    void onConfirmPress() override;
    bool onBackPress() override;
    bool onRightPress() override { return true; }

private:
    static const uint8_t ROW_COUNT = 6;

    Header header;
    ListRow rows[ROW_COUNT];
    Text statusText;

    Run &run;
    ScreenManager &screenManager;
    SoundManager &sound;
    Screen &playScreen;
    Screen &inventoryScreen;
    Screen &settingsScreen;
    Screen &abandonScreen;
    uint8_t selected = 0;

    void refreshRows();
};

#endif // PAUSE_SCREEN_H
