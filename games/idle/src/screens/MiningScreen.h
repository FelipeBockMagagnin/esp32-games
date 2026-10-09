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
#include "../game/Unlocks.h"
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

    // What opens the next screen, so a new player always knows what to aim for
    ProgressBar goalBar;

    GameState &game;
    const Unlocks &unlocks;
    SoundManager &sound;
    uint8_t shownTier; // Ore tier the tier-dependent elements were last set for
    bool prompting;    // The next-ore line is showing a prompt or message instead

    // Gold veins: the Mining screen is where they show and where Up catches them
    uint32_t seenVeinSpawns;       // So a vein chimes once, when it appears
    unsigned long veinMessageUntil; // What the last caught vein gave stays on the line until then
    String veinMessage;
    uint16_t veinMessageColor;

    unsigned long coinPulseUntil;
    void pulseCoin();

    void applyOreTier(uint8_t tier);
    void refreshVein(unsigned long now);

public:
    MiningScreen(GameState &game, const Unlocks &unlocks, SoundManager &sound);

    void onEnter(TFT_eSPI &tft) override;
    void update(unsigned long now) override;

    // Confirm mines gold, Up catches a gold vein
    void onConfirmPress() override;
    void onUpPress() override;
};

#endif // MINING_SCREEN_H
