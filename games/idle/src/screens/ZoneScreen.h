#ifndef ZONE_SCREEN_H
#define ZONE_SCREEN_H

#include "Screen.h"
#include "../ui/Text.h"
#include "../ui/Header.h"
#include "../ui/Image.h"
#include "../ui/ListRow.h"
#include "../ui/ListView.h"
#include "../assets/Assets.h"
#include "../game/GameState.h"
#include "../game/Inventory.h"
#include "../game/CombatState.h"
#include "../managers/ScreenManager.h"
#include "../managers/SoundManager.h"

// Picks the fighting zone. Kept apart from the combat screen because both of that
// screen's buttons are taken by the strike and guard actions.
class ZoneScreen : public Screen
{
private:
    Header header;

    ListRow rows[ListView::VISIBLE_ROWS];
    ListView list;

    // The player's own numbers, so the enemy stats on each row can be judged against them
    Image attackIcon;
    Text attackText;
    Image defenseIcon;
    Text defenseText;
    Image hpIcon;
    Text hpText;

    GameState &game;
    Inventory &inventory;
    CombatState &combat;
    SoundManager &sound;
    ScreenManager &screens;

    unsigned long lastRefresh;

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
