#include "MiningScreen.h"
#include "../game/Format.h"

static const unsigned long REFRESH_MS = 100;
static const uint16_t RATE_COLOR = 0xAD55; // Light gray, secondary to the ore amounts

MiningScreen::MiningScreen(GameState &game, SoundManager &sound)
    : // Header
      titleMining(90, 11, "Mining", 0xFFFF, 2),
      navUpgradeText(189, 5, "Upgrade", 0xFFFF, 1),
      navUpgradeArrow(234, 7, 3, 5, image_ButtonRightSmall_bits, 0xFFFF),

      // Ore type indicators
      copperIndicator(15, 39, 3, 3, getOreInfo(OreEnum::COPPER).color),
      copperLabel(24, 36, getOreInfo(OreEnum::COPPER).name, 0xFFFF, 1),
      silverIndicator(98, 40, 3, 3, getOreInfo(OreEnum::SILVER).color),
      silverLabel(107, 37, getOreInfo(OreEnum::SILVER).name, 0xFFFF, 1),
      goldIndicator(183, 40, 3, 3, getOreInfo(OreEnum::GOLD).color),
      goldLabel(192, 37, getOreInfo(OreEnum::GOLD).name, 0xFFFF, 1),
      copperRate(18, 48, "+0/s", RATE_COLOR, 1),
      silverRate(101, 49, "+0/s", RATE_COLOR, 1),
      goldRate(186, 49, "+0/s", RATE_COLOR, 1),

      // Center ore display
      oreImage(72, 87, 96, 96, image_Icon31_33_pixels),
      currentOreText(86, 179, getOreInfo(OreEnum::COPPER).name, 0xFFFF, 2),

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
    // Header
    addElement(&titleMining);
    addElement(&navUpgradeText);
    addElement(&navUpgradeArrow);

    // Ore indicators
    addElement(&copperIndicator);
    addElement(&copperLabel);
    addElement(&silverIndicator);
    addElement(&silverLabel);
    addElement(&goldIndicator);
    addElement(&goldLabel);
    addElement(&copperRate);
    addElement(&silverRate);
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

    // Ore labels show the stock of each ore; the dot color identifies which one
    copperLabel.setText(formatAmount(game.getOre(OreEnum::COPPER)));
    silverLabel.setText(formatAmount(game.getOre(OreEnum::SILVER)));
    goldLabel.setText(formatAmount(game.getOre(OreEnum::GOLD)));

    copperRate.setText(formatPerSecond(game.getProductionPerSecond(OreEnum::COPPER)));
    silverRate.setText(formatPerSecond(game.getProductionPerSecond(OreEnum::SILVER)));
    goldRate.setText(formatPerSecond(game.getProductionPerSecond(OreEnum::GOLD)));

    // The filled dot marks the ore being mined
    OreEnum current = game.getCurrentOre();
    copperIndicator.setFilled(current == OreEnum::COPPER);
    silverIndicator.setFilled(current == OreEnum::SILVER);
    goldIndicator.setFilled(current == OreEnum::GOLD);

    currentOreText.setText(getOreInfo(current).name);
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

void MiningScreen::onSelectPress()
{
    uint8_t next = (static_cast<uint8_t>(game.getCurrentOre()) + 1) % ORE_COUNT;
    game.setCurrentOre(static_cast<OreEnum>(next));
    lastRefresh = 0;
}
