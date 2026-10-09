#ifndef ASCEND_SCREEN_H
#define ASCEND_SCREEN_H

#include "Screen.h"
#include "../ui/Box.h"
#include "../ui/Text.h"
#include "../ui/Header.h"
#include "../ui/ListRow.h"
#include "../ui/ListView.h"
#include "../ui/ProgressBar.h"
#include "../ui/ButtonBadge.h"
#include "../game/GameState.h"
#include "../game/Ascension.h"
#include "../managers/ScreenManager.h"
#include "../managers/SoundManager.h"

// The ascension points and the talent tree they buy. The list's first row is the ascension
// itself, which Confirm hands to a Yes/No modal; the talents follow, each indented under the
// one it needs. Confirm on a talent buys its next level.
class AscendScreen : public Screen
{
private:
    static const uint8_t ROWS = ListView::VISIBLE_ROWS - 3;
    static const uint8_t LIST_COUNT = TALENT_COUNT + 1; // The ascend row, then the talents
    static const uint8_t NO_ITEM = 0xFF;

    Header header;

    Box summaryCard;
    Text pointsText;
    Text ascensionsText;
    Text bonusText;
    ProgressBar nextPointBar;

    ListRow rows[ROWS];
    ListView list;
    uint8_t rowItem[ROWS]; // Which item each row shows, so its fixed texts are not rebuilt

    Box detailPanel;
    Text statusText;
    Text levelText;
    Text effectText;
    ButtonBadge actionBadge;
    Text actionLabel;
    Text priceText;

    const GameState &game;
    Ascension &ascension;
    SoundManager &sound;
    ScreenManager &screenManager;
    Screen &confirmScreen;

    void refreshRow(ListRow &row, uint8_t item, bool selected, uint8_t slot);
    void refreshDetail();
    void refreshAscendDetail();
    void formatEffect(uint8_t id, char *buf, size_t size) const;

public:
    AscendScreen(const GameState &game, Ascension &ascension, SoundManager &sound, ScreenManager &screenManager,
                 Screen &confirmScreen);

    void update(unsigned long now) override;

    void onUpPress() override;
    void onDownPress() override;
    void onConfirmPress() override;
};

#endif // ASCEND_SCREEN_H
