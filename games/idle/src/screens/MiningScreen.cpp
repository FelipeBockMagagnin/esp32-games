#include "MiningScreen.h"
#include "../game/Format.h"

static const unsigned long REFRESH_MS = 100;
static const uint16_t RATE_COLOR = 0xAD55; // Light gray, secondary to the ore amounts

MiningScreen::MiningScreen(GameState &game, SoundManager &sound)
    : header("Mining", "", "Buildings"),

      // Gold balance
      goldIndicator(12, 40, 3, 3, GOLD_COLOR, true),
      goldLabel(21, 37, "0", 0xFFFF, 1),
      goldRate(15, 49, "+0/s", RATE_COLOR, 1),

      // Center ore display
      oreImage(72, 87, 96, 96, image_Icon31_33_pixels),
      currentOreText(96, 179, "Gold", 0xFFFF, 2),

      // Progress
      levelText(99, 231, "Level XX", 0xFFFF, 1),
      expBox(76, 243, 90, 13, 0xFFFF),
      expText(96, 246, "23/10000", 0xFFFF, 1),

      // Bottom sensors
      brightnessIcon(7, 298, 15, 16, image_display_brightness_bits, 0xFFFF),
      brightnessText(25, 302, "Light", 0xFFFF, 1),
      temperatureIcon(101, 298, 16, 16, image_weather_temperature_bits, 0xFFFF),
      temperatureText(119, 304, "temp", 0xFFFF, 1),
      humidityIcon(196, 297, 11, 16, image_weather_humidity_white_bits, 0xFFFF),
      humidityText(212, 304, "um", 0xFFFF, 1),

      game(game),
      sound(sound),
      lastRefresh(0)
{
    addElement(&header);

    // Gold balance
    addElement(&goldIndicator);
    addElement(&goldLabel);
    addElement(&goldRate);

    // Center ore display
    addElement(&oreImage);
    addElement(&currentOreText);

    // Progress
    addElement(&levelText);
    addElement(&expBox);
    addElement(&expText);

    // Bottom sensors
    addElement(&brightnessIcon);
    addElement(&brightnessText);
    addElement(&temperatureIcon);
    addElement(&temperatureText);
    addElement(&humidityIcon);
    addElement(&humidityText);
}

void MiningScreen::update(unsigned long now)
{
    if (lastRefresh != 0 && now - lastRefresh < REFRESH_MS)
    {
        return;
    }
    lastRefresh = now;

    goldLabel.setText(formatAmount(game.getGold()));
    goldRate.setText(formatPerSecond(game.getProductionPerSecond()));

    levelText.setText(String("Level ") + game.getMiningLevel());
    expText.setText(String(game.getMiningXp()) + "/" + game.getXpToNextLevel());
}

void MiningScreen::onConfirmPress()
{
    if (game.mine())
    {
        sound.playLevelUp();
    }
    else
    {
        sound.playMine();
    }
    lastRefresh = 0;
}
