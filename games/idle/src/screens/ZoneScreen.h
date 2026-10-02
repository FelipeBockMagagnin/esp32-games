#ifndef ZONE_SCREEN_H
#define ZONE_SCREEN_H

#include "Screen.h"
#include "../ui/Box.h"
#include "../ui/Text.h"
#include "../ui/Header.h"
#include "../ui/Image.h"
#include "../ui/ListRow.h"
#include "../ui/ListView.h"
#include "../ui/IconStrip.h"
#include "../ui/ButtonBadge.h"
#include "../assets/Assets.h"
#include "../game/GameState.h"
#include "../game/Inventory.h"
#include "../game/CombatState.h"
#include "../game/Stats.h"
#include "../managers/ScreenManager.h"
#include "../managers/SoundManager.h"

// Picks the fighting zone. Kept apart from the combat screen because both of that
// screen's buttons are taken by the strike and guard actions.
//
// Each row says how the player's current gear fares there (idle, needs the strike or
// smite, or out of reach), and the panel below spells it out per enemy along with the
// zone's drops, so gearing up has a visible target.
class ZoneScreen : public Screen
{
private:
    // Every zone fits on screen with room left for the detail panel
    static const uint8_t ROWS = 5;
    static const uint8_t ENEMY_LINES = 2; // Zones have two enemies each; extras go unlisted

    Header header;

    // The player's own numbers, so the enemy stats can be judged against them
    Box statsCard;
    Image attackIcon;
    Text attackText;
    Image defenseIcon;
    Text defenseText;
    Image hpIcon;
    Text hpText;

    ListRow rows[ROWS];
    ListView list;

    // Detail panel for the selected zone
    Box detailPanel;
    Text enemiesCaption;
    Text statusText;
    Text enemyName[ENEMY_LINES];
    Text enemyStats[ENEMY_LINES];
    Text enemyDeal[ENEMY_LINES];
    Text dropsCaption;
    Text dropsFound;
    IconStrip dropIcons;
    ButtonBadge fightBadge;
    Text fightLabel;
    Text rewardText;

    GameState &game;
    Inventory &inventory;
    CombatState &combat;
    SoundManager &sound;
    ScreenManager &screens;

    unsigned long lastRefresh;

    void refreshRows(const Stats &stats);
    void refreshPanel(const Stats &stats);

public:
    ZoneScreen(GameState &game, Inventory &inventory, CombatState &combat,
               SoundManager &sound, ScreenManager &screens);

    void update(unsigned long now) override;

    // Up / Down cycles through zones, Confirm enters the highlighted one and moves to combat
    void onUpPress() override;
    void onDownPress() override;
    void onConfirmPress() override;
};

#endif // ZONE_SCREEN_H
