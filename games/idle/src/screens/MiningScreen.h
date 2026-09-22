#ifndef MINING_SCREEN_H
#define MINING_SCREEN_H

#include "Screen.h"
#include "../ui/Text.h"
#include "../ui/Header.h"
#include "../ui/Image.h"
#include "../ui/Ellipse.h"
#include "../ui/OreDisplay.h"
#include "../ui/ProgressBar.h"
#include "../assets/Assets.h"
#include "../game/GameState.h"
#include "../managers/SoundManager.h"

class MiningScreen : public Screen
{
private:
    Header header;

    // Gold balance and automatic production under it
    Ellipse goldIndicator;
    Text goldLabel;
    Text goldRate;

    // Center ore display, animated on each click
    OreDisplay oreDisplay;
    Text currentOreText;

    // Progress
    Text levelText;
    ProgressBar expBar;

    // Bottom sensors
    Image brightnessIcon;
    Text brightnessText;
    Image temperatureIcon;
    Text temperatureText;
    Image humidityIcon;
    Text humidityText;

    GameState &game;
    SoundManager &sound;
    unsigned long lastRefresh;

public:
    MiningScreen(GameState &game, SoundManager &sound);

    void update(unsigned long now) override;

    // Confirm mines gold
    void onConfirmPress() override;
};

#endif // MINING_SCREEN_H
