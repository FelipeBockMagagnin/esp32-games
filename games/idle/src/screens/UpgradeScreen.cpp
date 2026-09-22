#include "UpgradeScreen.h"
#include "../game/Format.h"

static const unsigned long REFRESH_MS = 100;

static const uint16_t RATE_COLOR = 0xAD55;     // Light gray, secondary to the gold amount
static const uint16_t SELECTED_COLOR = 0xFFE0; // Yellow
static const uint16_t BOUGHT_COLOR = 0x07E0;   // Green
static const uint16_t LOCKED_COLOR = 0x7BEF;   // Gray, not enough gold

static const int16_t SLOT_X[GOLD_UPGRADE_COUNT] = {9, 46, 83, 119, 156, 193};
static const int16_t SLOT_Y = 55;
static const int16_t SLOT_W = 32;
static const int16_t SLOT_H = 31;
// Icon offset inside each slot box
static const int16_t SLOT_ICON_DX = 11;
static const int16_t SLOT_ICON_DY = 9;

// Size 1 characters are 6px wide, so this many fit inside the detail box
static const uint8_t DETAIL_LINE_CHARS = 34;

UpgradeScreen::UpgradeScreen(GameState &game, SoundManager &sound)
    : header("Upgrade", "Buildings", "Inventory"),

      goldIndicator(12, 40, 3, 3, GOLD_COLOR),
      goldText(21, 37, "0"),
      // Right-aligned on the gold line; the slots start right below it
      goldRate(229, 37, "+0/s", RATE_COLOR, 1, TR_DATUM),

      detailBox(11, 261, 218, 48, 0xFFFF),
      detailTitle(14, 265, "", 0xFFFF, 2),
      detailLine1(16, 283, ""),
      detailLine2(16, 293, ""),
      priceIndicator(190, 268, 3, 3, GOLD_COLOR),
      priceText(196, 265, ""),

      game(game),
      sound(sound),
      lastRefresh(0),
      selectedUpgrade(0)
{
    addElement(&header);

    addElement(&goldIndicator);
    addElement(&goldText);
    addElement(&goldRate);

    for (uint8_t i = 0; i < GOLD_UPGRADE_COUNT; i++)
    {
        slotBoxes[i] = Box(SLOT_X[i], SLOT_Y, SLOT_W, SLOT_H, 0xFFFF);
        slotIcons[i] = Image(SLOT_X[i] + SLOT_ICON_DX, SLOT_Y + SLOT_ICON_DY, 11, 16, image_cursor_black_white_bits, 0xFFFF);
        addElement(&slotBoxes[i]);
        addElement(&slotIcons[i]);
    }

    addElement(&detailBox);
    addElement(&detailTitle);
    addElement(&detailLine1);
    addElement(&detailLine2);
    addElement(&priceIndicator);
    addElement(&priceText);

    refreshDetails();
}

void UpgradeScreen::refreshDetails()
{
    const GoldUpgradeDef &def = GOLD_UPGRADES[selectedUpgrade];
    detailTitle.setText(def.name);

    // Wrap the description at the last space that fits on the first line
    String description = def.description;
    if (description.length() <= DETAIL_LINE_CHARS)
    {
        detailLine1.setText(description);
        detailLine2.setText("");
    }
    else
    {
        int split = description.lastIndexOf(' ', DETAIL_LINE_CHARS);
        if (split <= 0)
        {
            split = DETAIL_LINE_CHARS;
        }
        detailLine1.setText(description.substring(0, split));
        detailLine2.setText(description.substring(split + 1));
    }

    priceText.setText(game.isGoldUpgradeBought(selectedUpgrade) ? "Owned" : formatAmount(def.cost));
}

void UpgradeScreen::update(unsigned long now)
{
    if (lastRefresh != 0 && now - lastRefresh < REFRESH_MS)
    {
        return;
    }
    lastRefresh = now;

    goldText.setText(formatAmount(game.getGold()));
    goldRate.setText(formatPerSecond(game.getProductionPerSecond()));

    for (uint8_t i = 0; i < GOLD_UPGRADE_COUNT; i++)
    {
        uint16_t color = 0xFFFF;
        if (i == selectedUpgrade)
        {
            color = SELECTED_COLOR;
        }
        else if (game.isGoldUpgradeBought(i))
        {
            color = BOUGHT_COLOR;
        }
        else if (!game.canAffordGoldUpgrade(i))
        {
            color = LOCKED_COLOR;
        }
        slotBoxes[i].setBorderColor(color);
    }

    refreshDetails();
}

void UpgradeScreen::onSelectPress()
{
    selectedUpgrade = (selectedUpgrade + 1) % GOLD_UPGRADE_COUNT;
    lastRefresh = 0;
}

void UpgradeScreen::onConfirmPress()
{
    // buyGoldUpgrade only spends gold when the upgrade is not owned and affordable
    if (game.buyGoldUpgrade(selectedUpgrade))
    {
        sound.playBuy();
    }
    else
    {
        sound.playError();
    }
    lastRefresh = 0;
}
