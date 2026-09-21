#include "UpgradeScreen.h"
#include "../enum/OreEnum.h"
#include "../game/Format.h"

// Values change every frame while producing, so only rebuild the texts a few times per second
static const unsigned long REFRESH_MS = 100;

static const int16_t ORE_DISPLAY_X[ORE_COUNT] = {19, 102, 182};
static const int16_t ORE_DISPLAY_Y = 40;
static const int16_t ROW_X = 5;
static const int16_t FIRST_ROW_Y = 56;
static const int16_t ROW_SPACING = 38;

UpgradeScreen::UpgradeScreen(GameState &game, SoundManager &sound)
    : titleText(80, 12, "Upgrade", 0xFFFF, 2),
      headerLeftIcon(4, 7, 3, 5, image_ButtonLeftSmall_bits, 0xFFFF),
      backText(10, 5, "Mining"),
      headerRightIcon(235, 7, 3, 5, image_ButtonRightSmall_bits, 0xFFFF),
      nextText(197, 5, "Craft"),
      game(game),
      sound(sound),
      lastRefresh(0),
      selectedUpgrade(0)
{
    addElement(&titleText);

    addElement(&headerLeftIcon);
    addElement(&backText);
    addElement(&headerRightIcon);
    addElement(&nextText);

    for (uint8_t i = 0; i < ORE_COUNT; i++)
    {
        OreEnum ore = static_cast<OreEnum>(i);
        oreDisplays[i] = CoinDisplay(ORE_DISPLAY_X[i], ORE_DISPLAY_Y, "0", ore);
        addElement(&oreDisplays[i]);
    }

    for (uint8_t i = 0; i < UPGRADE_COUNT; i++)
    {
        const UpgradeDef &def = UPGRADES[i];
        upgradeRows[i] = UpgradeRow(ROW_X, FIRST_ROW_Y + i * ROW_SPACING, def.name,
                                    formatRate(def.productionPerLevel, def.producedOre));
        addElement(&upgradeRows[i]);
    }
}

void UpgradeScreen::update(unsigned long now)
{
    if (lastRefresh != 0 && now - lastRefresh < REFRESH_MS)
    {
        return;
    }
    lastRefresh = now;

    for (uint8_t i = 0; i < ORE_COUNT; i++)
    {
        oreDisplays[i].setText(formatAmount(game.getOre(static_cast<OreEnum>(i))));
    }

    for (uint8_t i = 0; i < UPGRADE_COUNT; i++)
    {
        UpgradeRow &row = upgradeRows[i];
        row.setCopperPrice(game.getUpgradeCost(i, OreEnum::COPPER));
        row.setSilverPrice(game.getUpgradeCost(i, OreEnum::SILVER));
        row.setGoldPrice(game.getUpgradeCost(i, OreEnum::GOLD));
        row.setBuyCount((uint32_t)game.getUpgradeLevel(i));
        row.setAffordable(game.canAfford(i));
        row.setSelected(i == selectedUpgrade);
    }
}

void UpgradeScreen::onSelectPress()
{
    selectedUpgrade = (selectedUpgrade + 1) % UPGRADE_COUNT;
    lastRefresh = 0;
}

void UpgradeScreen::onConfirmPress()
{
    // buyUpgrade only spends ore when the player can afford it
    if (game.buyUpgrade(selectedUpgrade))
    {
        sound.playBuy();
    }
    else
    {
        sound.playError();
    }
    lastRefresh = 0;
}
