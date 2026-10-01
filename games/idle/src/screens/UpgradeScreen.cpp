#include "UpgradeScreen.h"
#include "../game/Format.h"

static const unsigned long REFRESH_MS = 100;

static const uint16_t RATE_COLOR = 0xAD55;     // Light gray, secondary to the gold amount
static const uint16_t SELECTED_COLOR = 0xFFE0; // Yellow
static const uint16_t LOCKED_COLOR = 0x7BEF;   // Gray, not enough gold

static const uint8_t SLOTS_PER_ROW = 6;

static const int16_t SLOT_X[SLOTS_PER_ROW] = {9, 46, 83, 119, 156, 193};
static const int16_t SLOT_Y[2] = {55, 91};
static const int16_t SLOT_W = 32;
static const int16_t SLOT_H = 31;
// Icon offset inside each slot box
static const int16_t SLOT_ICON_DX = 11;
static const int16_t SLOT_ICON_DY = 9;

// Size 1 characters are 6px wide, so this many fit inside the detail box
static const uint8_t DETAIL_LINE_CHARS = 34;

UpgradeScreen::UpgradeScreen(GameState &game, SoundManager &sound)
    : header("Upgrade", "Buildings", "Awards"),

      goldIndicator(13, 40, 5, 5, GOLD_COLOR, true),
      goldText(21, 32, "0", 0xFFFF, 2),
      // Right-aligned on the gold line; the slots start right below it
      goldRate(229, 32, "+0/s", RATE_COLOR, 2, TR_DATUM),

      detailBox(11, 261, 218, 48, 0xFFFF),
      detailTitle(14, 265, "", 0xFFFF, 2),
      detailLine1(16, 283, ""),
      detailLine2(16, 293, ""),
      priceIndicator(190, 268, 3, 3, GOLD_COLOR),
      priceText(196, 265, ""),

      game(game),
      sound(sound),
      lastRefresh(0),
      visibleCount(0),
      selectedSlot(0)
{
    addElement(&header);
    setHeader(&header);

    addElement(&goldIndicator);
    addElement(&goldText);
    addElement(&goldRate);

    for (uint8_t i = 0; i < MAX_VISIBLE_SLOTS; i++)
    {
        int16_t slotX = SLOT_X[i % SLOTS_PER_ROW];
        int16_t slotY = SLOT_Y[i / SLOTS_PER_ROW];
        slotBoxes[i] = Box(slotX, slotY, SLOT_W, SLOT_H, 0xFFFF);
        slotIcons[i] = Image(slotX + SLOT_ICON_DX, slotY + SLOT_ICON_DY, 11, 16, image_cursor_black_white_bits, 0xFFFF);
        visibleIds[i] = 0;
        addElement(&slotBoxes[i]);
        addElement(&slotIcons[i]);
    }

    addElement(&detailBox);
    addElement(&detailTitle);
    addElement(&detailLine1);
    addElement(&detailLine2);
    addElement(&priceIndicator);
    addElement(&priceText);

    rebuildVisible();
    refreshDetails();
}

void UpgradeScreen::rebuildVisible()
{
    visibleCount = 0;
    for (uint8_t i = 0; i < GOLD_UPGRADE_COUNT && visibleCount < MAX_VISIBLE_SLOTS; i++)
    {
        if (game.isGoldUpgradeUnlocked(i) && !game.isGoldUpgradeBought(i))
        {
            visibleIds[visibleCount++] = i;
        }
    }

    // A purchase shrinks the list, so the selection can end up past the end
    if (selectedSlot >= visibleCount)
    {
        selectedSlot = visibleCount > 0 ? visibleCount - 1 : 0;
    }
}

void UpgradeScreen::refreshDetails()
{
    if (visibleCount == 0)
    {
        detailTitle.setText("No upgrades");
        detailLine1.setText("Buy buildings to unlock more");
        detailLine2.setText("");
        priceText.setText("");
        priceIndicator.setVisible(false);
        return;
    }

    priceIndicator.setVisible(true);

    const GoldUpgradeDef &def = GOLD_UPGRADES[visibleIds[selectedSlot]];
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

    priceText.setText(formatAmount(def.cost));
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

    rebuildVisible();

    for (uint8_t i = 0; i < MAX_VISIBLE_SLOTS; i++)
    {
        bool used = i < visibleCount;
        slotBoxes[i].setVisible(used);
        slotIcons[i].setVisible(used);
        if (!used)
        {
            continue;
        }

        uint16_t color = 0xFFFF;
        if (i == selectedSlot)
        {
            color = SELECTED_COLOR;
        }
        else if (!game.canAffordGoldUpgrade(visibleIds[i]))
        {
            color = LOCKED_COLOR;
        }
        slotBoxes[i].setBorderColor(color);
    }

    refreshDetails();
}

void UpgradeScreen::onSelectPress()
{
    if (visibleCount > 0)
    {
        selectedSlot = (selectedSlot + 1) % visibleCount;
    }
    lastRefresh = 0;
}

void UpgradeScreen::onConfirmPress()
{
    // buyGoldUpgrade only spends gold when the upgrade is unlocked, unowned and affordable
    if (visibleCount > 0 && game.buyGoldUpgrade(visibleIds[selectedSlot]))
    {
        sound.playBuy();
    }
    else
    {
        sound.playError();
    }
    lastRefresh = 0;
}
