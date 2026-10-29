#ifndef SHOP_SCREEN_H
#define SHOP_SCREEN_H

#include "Screen.h"
#include "../ui/Header.h"
#include "ListRow.h"
#include "../ui/Text.h"
#include "../jack/Profile.h"
#include "../core/SoundManager.h"

// One shelf of the Unity market, styles or songs, each bought once with coins and then worn or
// played. A screen of the rotation, so Left and Right leave it.
class ShopScreen : public Screen
{
public:
    // `changed` runs after a purchase or a change of style or song, to apply and save it
    ShopScreen(Shelf shelf, Profile &profile, SoundManager &sound, void (*changed)());

    void onEnter(TFT_eSPI &tft) override;
    void onUpPress() override;
    void onDownPress() override;
    void onConfirmPress() override;

private:
    static const uint8_t MAX_ROWS = STYLE_COUNT > SONG_COUNT ? STYLE_COUNT : SONG_COUNT;

    Header header;
    ListRow rows[MAX_ROWS];
    Text coinText;

    Shelf shelf;
    Profile &profile;
    SoundManager &sound;
    void (*changed)();
    uint8_t selected = 0;

    void refreshRows();
};

#endif // SHOP_SCREEN_H
