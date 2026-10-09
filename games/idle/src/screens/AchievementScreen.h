#ifndef ACHIEVEMENT_SCREEN_H
#define ACHIEVEMENT_SCREEN_H

#include "Screen.h"
#include "../ui/Box.h"
#include "../ui/Text.h"
#include "../ui/Image.h"
#include "../ui/Header.h"
#include "../ui/ListRow.h"
#include "../ui/ListView.h"
#include "../ui/ProgressBar.h"
#include "../ui/ButtonBadge.h"
#include "../assets/Assets.h"
#include "../game/Achievements.h"
#include "../game/GameState.h"
#include "../game/Inventory.h"
#include "../game/CombatState.h"
#include "../managers/SoundManager.h"

// Lists every achievement, unlocked or not, with the running total of how much gold
// production they have earned and, under the list, the combat bonuses the maxed gear and
// jewellery have paid. Up / Down browse the list, Confirm jumps to the next one
// still locked, which beats pressing through 35 rows. The panel under the list shows how
// far the selected one has got.
class AchievementScreen : public Screen
{
private:
    // Three rows fewer than the default, to make room for the bonus card and progress panel
    static const uint8_t ROWS = ListView::VISIBLE_ROWS - 3;
    static const uint8_t BONUS_CELLS = ACH_REWARD_COUNT - 1; // Every reward but production

    Header header;

    // Summary card: trophy, unlocked count, overall completion and the bonus earned
    Box summaryCard;
    Image trophyIcon;
    Text countText;
    Text bonusText;
    ProgressBar completionBar;

    ListRow rows[ROWS];
    ListView list;

    // One cell per combat reward, dimmed until something pays into it, so it also shows
    // what is left to earn
    Box bonusCard;
    Text bonusCells[BONUS_CELLS];

    // Which achievement each row shows, so its fixed texts are not rebuilt every refresh
    uint8_t rowAchievement[ROWS];

    // Progress panel for the selected achievement
    Box detailPanel;
    Text rewardText;
    Text statusText;
    ProgressBar progressBar;
    ButtonBadge nextBadge;
    Text nextLabel;
    ButtonBadge upBadge;
    ButtonBadge downBadge;
    Text browseLabel;

    Achievements &achievements;
    GameState &game;
    Inventory &inventory;
    CombatState &combat;
    SoundManager &sound;

    uint8_t lastUnlockedCount;

    void updateDetail();

public:
    AchievementScreen(Achievements &achievements, GameState &game, Inventory &inventory, CombatState &combat,
                      SoundManager &sound);

    void update(unsigned long now) override;

    void onUpPress() override;
    void onDownPress() override;
    void onConfirmPress() override;
};

#endif // ACHIEVEMENT_SCREEN_H
