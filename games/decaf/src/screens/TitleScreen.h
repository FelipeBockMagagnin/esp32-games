#ifndef TITLE_SCREEN_H
#define TITLE_SCREEN_H

#include "Screen.h"
#include "../ui/Text.h"
#include "../ui/SheetImage.h"
#include "../game/SaveGame.h"
#include "../managers/ScreenManager.h"
#include "../managers/SoundManager.h"

// The lobby: the title, the programmer with his empty mug, Start and Settings, and the best
// run on the first floor
class TitleScreen : public Screen
{
public:
    TitleScreen(ScreenManager &screenManager, SoundManager &sound, const SaveBlob &records, Screen &settingsScreen,
                void (*start)(uint8_t level));

    void onEnter(TFT_eSPI &tft) override;
    void update(unsigned long now) override;

    void onConfirmPress() override;
    void onUpPress() override;
    void onDownPress() override;
    bool onBackPress() override { return true; }
    bool onNextPress() override { return true; }

private:
    static const uint8_t MENU_ROWS = 2;

    ScreenManager &screenManager;
    SoundManager &sound;
    const SaveBlob &records;
    Screen &settingsScreen;
    void (*start)(uint8_t level);
    uint8_t selected;

    Text titleText;
    Text infernoText;
    Text subtitleText[2];
    Text menuRows[MENU_ROWS];
    Text bestText;
    SheetImage hero;

    void refreshMenu();
};

#endif // TITLE_SCREEN_H
