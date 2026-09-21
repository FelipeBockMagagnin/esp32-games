#include "MiningScreen.h"

MiningScreen::MiningScreen()
    : // Header
      titleMining(90, 11, "Mining", 0xFFFF, 2),
      navUpgradeText(189, 5, "Upgrade", 0xFFFF, 1),
      navUpgradeArrow(234, 7, 3, 5, image_ButtonRightSmall_bits, 0xFFFF),

      // Ore type indicators
      cooperIndicator(15, 39, 3, 3, 0xE300),
      cooperLabel(24, 36, "Cooper", 0xFFFF, 1),
      silverIndicator(98, 40, 3, 3, 0x73AF),
      silverLabel(107, 37, "Silver", 0xFFFF, 1),
      goldIndicator(183, 40, 3, 3, 0xF685),
      goldLabel(192, 37, "Gold", 0xFFFF, 1),

      // Center ore display
      oreImage(72, 87, 96, 96, image_Icon31_33_pixels),
      currentOreText(86, 179, "Cooper", 0xFFFF, 2),

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
      humidityText(212, 304, "um", 0xFFFF, 1)
{
    // Header
    addElement(&titleMining);
    addElement(&navUpgradeText);
    addElement(&navUpgradeArrow);

    // Ore indicators
    addElement(&cooperIndicator);
    addElement(&cooperLabel);
    addElement(&silverIndicator);
    addElement(&silverLabel);
    addElement(&goldIndicator);
    addElement(&goldLabel);

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
    // No dynamic updates yet
}
