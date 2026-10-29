#ifndef INVENTORY_SCREEN_H
#define INVENTORY_SCREEN_H

#include "Screen.h"
#include "Header.h"
#include "ListRow.h"
#include "Text.h"
#include "../core/ScreenManager.h"
#include "../core/SoundManager.h"
#include "../crawl/Run.h"

// A modal from the pause menu: what is worn on the left, the bag on the right. The cursor walks
// the bag; the worn item of the same slot lights up beside it, and an arrow says which is better.
class InventoryScreen : public Screen
{
public:
    InventoryScreen(Run &run, ScreenManager &screenManager, SoundManager &sound, Screen &pauseScreen);

    void onEnter(TFT_eSPI &tft) override;
    void onUpPress() override;
    void onDownPress() override;
    void onConfirmPress() override;
    bool onBackPress() override;
    bool onRightPress() override;

private:
    Header header;
    ListRow worn[SLOT_COUNT];
    ListRow bag[BAG_SIZE];
    Text hintText;
    Text potionText;

    Run &run;
    ScreenManager &screenManager;
    SoundManager &sound;
    Screen &pauseScreen;
    uint8_t selected = 0;

    void refresh();
    void fill(ListRow &row, const Item &item, const char *emptyTitle);
};

#endif // INVENTORY_SCREEN_H
