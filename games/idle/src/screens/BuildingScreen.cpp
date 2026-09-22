#include "BuildingScreen.h"
#include "../game/Format.h"

// Values change every frame while producing, so only rebuild the texts a few times per second
static const unsigned long REFRESH_MS = 100;

static const int16_t ROW_X = 5;
static const int16_t FIRST_ROW_Y = 56;
static const int16_t ROW_SPACING = 38;

static const uint16_t RATE_COLOR = 0xAD55; // Light gray, secondary to the gold amount

BuildingScreen::BuildingScreen(GameState &game, SoundManager &sound)
    : header("Buildings", "Mining", "Upgrade"),
      goldIndicator(13, 40, 5, 5, GOLD_COLOR, true),
      goldText(21, 32, "0", 0xFFFF, 2),
      goldRate(229, 32, "+0/s", RATE_COLOR, 2, TR_DATUM),
      game(game),
      sound(sound),
      lastRefresh(0),
      selectedBuilding(0)
{
    addElement(&header);

    addElement(&goldIndicator);
    addElement(&goldText);
    addElement(&goldRate);

    for (uint8_t i = 0; i < BUILDING_COUNT; i++)
    {
        const BuildingsDef &def = BUILDINGS[i];
        buildingRows[i] = UpgradeRow(ROW_X, FIRST_ROW_Y + i * ROW_SPACING, def.name,
                                     formatRate(def.productionPerLevel));
        addElement(&buildingRows[i]);
    }
}

void BuildingScreen::update(unsigned long now)
{
    if (lastRefresh != 0 && now - lastRefresh < REFRESH_MS)
    {
        return;
    }
    lastRefresh = now;

    goldText.setText(formatAmount(game.getGold()));
    goldRate.setText(formatPerSecond(game.getProductionPerSecond()));

    for (uint8_t i = 0; i < BUILDING_COUNT; i++)
    {
        UpgradeRow &row = buildingRows[i];
        row.setPrice(game.getBuildingCost(i));
        row.setBuyCount((uint32_t)game.getBuildingLevel(i));
        row.setAffordable(game.canAffordBuilding(i));
        row.setSelected(i == selectedBuilding);
    }
}

void BuildingScreen::onSelectPress()
{
    selectedBuilding = (selectedBuilding + 1) % BUILDING_COUNT;
    lastRefresh = 0;
}

void BuildingScreen::onConfirmPress()
{
    // buyBuilding only spends gold when the player can afford it
    if (game.buyBuilding(selectedBuilding))
    {
        sound.playBuy();
    }
    else
    {
        sound.playError();
    }
    lastRefresh = 0;
}
