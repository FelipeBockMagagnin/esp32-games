#include "MiningScreen.h"
#include "../game/Format.h"

static const unsigned long REFRESH_MS = 100;
static const uint16_t RATE_COLOR = 0xAD55;   // Light gray, secondary to the ore amounts
static const uint16_t XP_BAR_COLOR = 0x07FF; // Cyan, distinct from the gold/ore amber tones

MiningScreen::MiningScreen(GameState &game, SoundManager &sound)
    : header("Mining", "", "Buildings"),

      // Gold balance; the dot and rate stay small, the amount is the screen's headline number
      goldIndicator(13, 40, 5, 5, GOLD_COLOR, true),
      goldLabel(24, 32, "0", 0xFFFF, 2),
      goldRate(24, 49, "+0/s", RATE_COLOR, 1),

      // Center ore display
      // Covers the empty space above the ore so popups can float into it
      oreDisplay(48, 58, 144, 128, image_Icon31_33_pixels, 24, 29, 96, 96, 0xFFFF),
      // Centered: the ore name's length changes with the tier ("Gold Ore" vs "Mythril Ore")
      currentOreText(120, 188, ORE_TIERS[0].name, 0xFFFF, 2, TC_DATUM),

      // Progress: level number above an XP bar that fills as the player mines
      levelText(120, 222, "Level 1", 0xFFFF, 2, TC_DATUM),
      expBar(75, 241, 90, 15, 0, XP_PER_LEVEL, XP_BAR_COLOR),

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
    addElement(&oreDisplay);
    addElement(&currentOreText);

    // Progress
    addElement(&levelText);
    addElement(&expBar);

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
    // Animations run at their own frame rate, independent of the text refresh below
    oreDisplay.update(now);

    if (lastRefresh != 0 && now - lastRefresh < REFRESH_MS)
    {
        return;
    }
    lastRefresh = now;

    goldLabel.setText(formatAmount(game.getGold()));
    goldRate.setText(formatPerSecond(game.getProductionPerSecond()));

    currentOreText.setText(game.getOreTierName());
    levelText.setText(String("Level ") + game.getMiningLevel());

    uint32_t xp = game.getMiningXp();
    uint32_t xpToNext = game.getXpToNextLevel();
    expBar.setProgress(xp, xpToNext);
    expBar.setLabel(String(xp) + "/" + xpToNext);
}

void MiningScreen::onConfirmPress()
{
    bool leveledUp;
    uint32_t gained = game.mine(leveledUp);

    oreDisplay.shake();
    oreDisplay.addPopup(String("+") + formatAmount(gained));

    if (leveledUp)
    {
        sound.playLevelUp();
    }
    else
    {
        sound.playMine();
    }
    lastRefresh = 0;
}
