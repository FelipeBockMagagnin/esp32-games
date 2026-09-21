#ifndef MINING_SCREEN_H
#define MINING_SCREEN_H

#include "Screen.h"
#include "../ui/Text.h"
#include "../ui/Box.h"
#include "../ui/Image.h"
#include "../ui/Ellipse.h"
#include "../assets/Assets.h"
#include "../enum/OreEnum.h"

class MiningScreen : public Screen
{
private:
    // Header
    Text titleMining;
    Text navUpgradeText;
    Image navUpgradeArrow;

    // Ore type indicators
    Ellipse copperIndicator;
    Text copperLabel;
    Ellipse silverIndicator;
    Text silverLabel;
    Ellipse goldIndicator;
    Text goldLabel;

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

public:
    MiningScreen();

    void update(unsigned long now) override;
};

#endif // MINING_SCREEN_H
