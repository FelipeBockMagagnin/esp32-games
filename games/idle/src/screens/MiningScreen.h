#ifndef MINING_SCREEN_H
#define MINING_SCREEN_H

#include "Screen.h"
#include "../ui/Text.h"
#include "../ui/Header.h"
#include "../ui/Box.h"
#include "../ui/Image.h"
#include "../ui/Ellipse.h"
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

    // Center ore display
    Image oreImage;
    Text currentOreText;

    // Progress
    Text levelText;
    Box expBox;
    Text expText;

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
