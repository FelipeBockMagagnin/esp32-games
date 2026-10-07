#ifndef MINING_SCREEN_H
#define MINING_SCREEN_H

#include "Screen.h"
#include "../ui/Box.h"
#include "../ui/Text.h"
#include "../ui/Header.h"
#include "../ui/Ellipse.h"
#include "../ui/OreDisplay.h"
#include "../ui/ProgressBar.h"
#include "../ui/ButtonBadge.h"
#include "../assets/Assets.h"
#include "../game/GameState.h"
#include "../managers/SoundManager.h"

class MiningScreen : public Screen
{
private:
    Header header;

    // Gold card: balance as the headline, automatic production under it
    Box goldCard;
    Ellipse goldCoin;
    Text goldLabel;
    Text goldRate;

    // Center ore display, animated on each click and recolored per ore tier
    OreDisplay oreDisplay;
    Text currentOreText;
    Text nextOreText;

    // Mining level and XP towards the next one
    Text levelText;
    Text xpText;
    ProgressBar expBar;

    // What Confirm does, and how much it pays
    ButtonBadge mineBadge;
    Text mineLabel;
    Text mineValue;

    GameState &game;
    SoundManager &sound;
    unsigned long lastRefresh;
    uint8_t shownTier; // Ore tier the tier-dependent elements were last set for

    void applyOreTier(uint8_t tier);

public:
    MiningScreen(GameState &game, SoundManager &sound);

    void update(unsigned long now) override;

    // Confirm mines gold
    void onConfirmPress() override;
};

#endif // MINING_SCREEN_H
