#ifndef UPGRADE_SCREEN_H
#define UPGRADE_SCREEN_H

#include "Screen.h"
#include "../ui/PressFeedback.h"
#include "../ui/Text.h"
#include "../ui/Header.h"
#include "../ui/Box.h"
#include "../ui/Ellipse.h"
#include "../ui/ProgressBar.h"
#include "../ui/ButtonBadge.h"
#include "../ui/ListRow.h"
#include "../ui/ListView.h"
#include "../assets/Assets.h"
#include "../game/GameState.h"
#include "../managers/SoundManager.h"

class UpgradeScreen : public Screen
{
private:
    // Two rows fewer than the default, to make room for the detail panel at the bottom
    static const uint8_t ROWS = ListView::VISIBLE_ROWS - 2;

    Header header;

    Box goldCard;
    Ellipse goldCoin;
    Text goldText;
    Text goldRate;

    ListRow rows[ROWS];
    ListView list;
    uint8_t rowUpgrade[ROWS];

    Box detailPanel;
    Text categoryText;
    Text countText;
    Text detailLine1;
    Text detailLine2;
    ButtonBadge buyBadge;
    Text buyLabel;
    Text priceText;
    ProgressBar savingsBar;

    GameState &game;
    SoundManager &sound;
    PressFeedback feedback;
    unsigned long spentUntil = 0; // The balance blinks white as a purchase spends it

    // Only unlocked, unbought upgrades are listed; the list's indices point into this
    uint8_t visibleIds[GOLD_UPGRADE_COUNT];
    uint8_t visibleCount;
    uint8_t shownUpgrade; // Upgrade the detail panel's fixed texts were last written for

    void rebuildVisible();
    void refreshDetails();
    void showDetailElements(bool visible);
    void formatEffect(const GoldUpgradeDef &def, char *buf, size_t size) const;

public:
    UpgradeScreen(GameState &game, SoundManager &sound);

    void update(unsigned long now) override;

    // Up / Down buttons navigate through upgrades, confirm button buys the selected one
    void onUpPress() override;
    void onDownPress() override;
    void onConfirmPress() override;
};

#endif // UPGRADE_SCREEN_H
