#ifndef UPGRADE_SCREEN_H
#define UPGRADE_SCREEN_H

#include "Screen.h"
#include "../ui/Text.h"
#include "../ui/Header.h"
#include "../ui/Box.h"
#include "../ui/Ellipse.h"
#include "../ui/ProgressBar.h"
#include "../ui/ButtonBadge.h"
#include "../ui/UpgradeSlot.h"
#include "../assets/Assets.h"
#include "../game/GameState.h"
#include "../managers/SoundManager.h"

class UpgradeScreen : public Screen
{
public:
    // The grid holds three rows of six; upgrades beyond that wait until a slot frees up
    static const uint8_t MAX_VISIBLE_SLOTS = 18;

private:
    Header header;

    // Gold card: balance and production, as on the Mining and Buildings screens
    Box goldCard;
    Ellipse goldCoin;
    Text goldText;
    Text goldRate;

    // Upgrade grid, each tile tinted and iconed by its upgrade's category
    UpgradeSlot slots[MAX_VISIBLE_SLOTS];

    // Selected upgrade details
    Box detailPanel;
    Text categoryText;
    Text countText;
    Text detailTitle;
    Text detailLine1;
    Text detailLine2;
    Box detailDivider;
    Text effectText;
    ProgressBar savingsBar;
    ButtonBadge buyBadge;
    Text buyLabel;
    Text priceText;

    GameState &game;
    SoundManager &sound;
    unsigned long lastRefresh;

    // Only unlocked, unbought upgrades get a slot, so the grid stays small as the
    // upgrade table grows. selectedSlot indexes this list, not GOLD_UPGRADES.
    uint8_t visibleIds[MAX_VISIBLE_SLOTS];
    uint8_t visibleCount;
    uint8_t availableCount; // Unlocked and unbought, including any that did not fit the grid
    uint8_t selectedSlot;
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
