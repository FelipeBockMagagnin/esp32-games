#ifndef SETTINGS_SCREEN_H
#define SETTINGS_SCREEN_H

#include "Screen.h"
#include "Header.h"
#include "ListRow.h"
#include "Settings.h"
#include "../app/Transport.h"
#include "../core/ScreenManager.h"

// Theme, play mode, screen dimming, the credits and, when the board has one, the way back to the
// game menu. Left goes back to the player.
class SettingsScreen : public Screen
{
public:
    // `changed` runs after every change, to apply and store the settings; `toMenu` boots the
    // game menu
    SettingsScreen(Settings &settings, void (*changed)(), void (*toMenu)(), Transport &transport,
                   ScreenManager &screenManager, Screen &credits);

    void onEnter(TFT_eSPI &tft) override;
    void onUpPress() override;
    void onDownPress() override;
    void onConfirmPress() override;
    bool onBackPress() override;
    bool onRightPress() override;

private:
    static const uint8_t ROW_COUNT = 5;

    Header header;
    ListRow rows[ROW_COUNT];
    Settings &settings;
    void (*changed)();
    void (*toMenu)();
    Transport &transport;
    ScreenManager &screenManager;
    Screen &credits;
    uint8_t selected = 0;
    bool menuShown = false;

    void step(int8_t delta);
    void refreshRows();
};

#endif // SETTINGS_SCREEN_H
