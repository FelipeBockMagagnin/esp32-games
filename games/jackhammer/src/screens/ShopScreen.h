#ifndef SHOP_SCREEN_H
#define SHOP_SCREEN_H

#include "Screen.h"
#include "../ui/Header.h"
#include "../ui/ListRow.h"
#include "../ui/Text.h"
#include "../jack/Profile.h"
#include "../core/ScreenManager.h"
#include "../core/SoundManager.h"

// The Unity market: styles and songs, each bought once with coins and then worn or played.
// A modal from the title; Right switches shelf, Left goes back.
class ShopScreen : public Screen
{
public:
    // `changed` runs after a purchase or a change of style or song, to apply and save it
    ShopScreen(Profile &profile, ScreenManager &screenManager, SoundManager &sound, void (*changed)());

    void onEnter(TFT_eSPI &tft) override;
    void onUpPress() override;
    void onDownPress() override;
    void onConfirmPress() override;
    bool onBackPress() override;
    bool onRightPress() override;

private:
    static const uint8_t MAX_ROWS = STYLE_COUNT > SONG_COUNT ? STYLE_COUNT : SONG_COUNT;

    Header header;
    ListRow rows[MAX_ROWS];
    Text coinText;
    Text hintText;

    Profile &profile;
    ScreenManager &screenManager;
    SoundManager &sound;
    void (*changed)();
    Shelf shelf = Shelf::STYLES;
    uint8_t selected = 0;

    void build();
    void refreshRows();
};

#endif // SHOP_SCREEN_H
