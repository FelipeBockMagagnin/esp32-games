#ifndef COMBAT_SCREEN_H
#define COMBAT_SCREEN_H

#include "Screen.h"
#include "../ui/Text.h"
#include "../ui/Header.h"
#include "../ui/Image.h"
#include "../ui/Box.h"
#include "../ui/ButtonBadge.h"
#include "../ui/ProgressBar.h"
#include "../assets/Assets.h"
#include "../game/CombatState.h"
#include "../managers/SoundManager.h"

// Shows the fight that CombatState is already running. Three actions sit between the
// enemy block and the player block: Confirm strikes, Up smites (with amulet), and Down guards.
// Laid out with the enemy sprite and its stat card side-by-side above, action buttons
// clearly labeled with button badges in the middle, and the player status below.
class CombatScreen : public Screen
{
private:
    Header header;

    // Enemy block
    Text enemyName;
    Image enemySprite;
    Box enemyStatsBox;
    Box enemyStatsBand; // Filled title strip across the top of the card
    Text enemyStatsHeader;
    Image enemyAtkIcon;
    Text enemyAtkText;
    Image enemyDefIcon;
    Text enemyDefText;
    Box enemyDropsDivider;
    Text enemyDropsLabel;
    // Drop pool as a 2-column grid of item icon + chance, indexed by drop slot and hidden
    // when unused. The chance is green for an item not owned yet and gold for one that
    // would only level up, matching the colors of the drop notification.
    static const uint8_t MAX_DROPS = 6;
    Image dropIcon[MAX_DROPS];
    Text dropPct[MAX_DROPS];

    ProgressBar enemyHpBar;
    ProgressBar enemyAttackBar; // Drains towards the enemy's next hit

    // Dedicated status/drop notification line
    Text messageText;

    // Action panel: one row per button, laid out top to bottom like the physical
    // Up / Confirm / Down. Each row is the button's drawing, the action name, what it
    // does right now (multiplier and effective damage, or the guard's effect), and
    // whether it is ready.
    Box actionBox;
    ButtonBadge smiteBadge;
    Text smiteLabel;
    Text smiteDetail;
    Text smiteState;
    ButtonBadge strikeBadge;
    Text strikeLabel;
    Text strikeDetail;
    Text strikeState;
    ButtonBadge guardBadge;
    Text guardLabel;
    Text guardDetail;
    Text guardState;

    // Player block
    ProgressBar playerAttackBar; // Hidden entirely when it cannot damage the enemy
    ProgressBar playerHpBar;
    Text statsLine;

    CombatState &combat;
    SoundManager &sound;

    unsigned long lastRefresh;
    uint32_t seenKills;
    uint32_t seenDrops;
    unsigned long messageUntil; // A drop notification holds the message line until this time

    void showFightElements(bool visible);
    void hideDrops();
    void refreshDrops(const EnemyDef &enemy);
    void showDropMessage(unsigned long now);
    void refreshActionStates(unsigned long now, bool fighting);
    void refreshTimerBars(unsigned long now, bool fighting);

public:
    CombatScreen(CombatState &combat, SoundManager &sound);

    void onEnter(TFT_eSPI &tft) override;
    void update(unsigned long now) override;

    void onConfirmPress() override;
    void onUpPress() override;
    void onDownPress() override;
};

#endif // COMBAT_SCREEN_H
