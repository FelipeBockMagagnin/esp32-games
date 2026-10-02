#include "UpgradeScreen.h"
#include "../game/Format.h"
#include "../game/CombatConfig.h" // ZONES, for the zone an unlock opens

static const unsigned long REFRESH_MS = 100;

static const uint16_t CARD_BORDER = 0x7AE2; // Dark amber, same frame as the other gold cards
static const uint16_t RATE_COLOR = 0x7EEF;  // Soft green: income that grows on its own
static const uint16_t PANEL_BORDER = 0x31A6; // Dark slate
static const uint16_t DIVIDER_COLOR = 0x3187;
static const uint16_t LINE_COLOR = 0xCE59;  // Description, a step below the white title
static const uint16_t HINT_COLOR = 0x94B2;
static const uint16_t AFFORD_COLOR = 0x07E0;
static const uint16_t SAVINGS_COLOR = 0x9B00; // Amber fill, dark enough for the white label
static const uint16_t SAVINGS_BG = 0x2080;
static const uint16_t SAVINGS_BORDER = 0x7AE2;
static const uint16_t READY_FILL = 0x0400; // Dark green once the price is covered

// How each upgrade category looks, indexed by UpgradeTarget. The colors echo where the
// bonus lands: gold for production, the combat screen's red/blue/green for its stats.
struct CategoryStyle
{
    const char *label;
    uint16_t accent;
    const uint16_t *icon;
};

static const CategoryStyle CATEGORY_STYLES[] = {
    {"PRODUCTION", GOLD_COLOR, image_upgrade_production_pixels}, // PRODUCTION
    {"MINING", 0xFC60, image_upgrade_click_pixels},               // CLICK
    {"ATTACK", 0xFB2C, image_sword_02b_pixels},                   // ATTACK
    {"DEFENSE", 0x6E7F, image_armor_01b_pixels},                  // DEFENSE
    {"HEALTH", 0x2D45, image_upgrade_heart_pixels},               // MAX_HP
    {"NEW ZONE", 0xB3DF, image_upgrade_map_pixels},               // UNLOCK_ZONE
};

static const CategoryStyle &styleFor(UpgradeTarget target)
{
    return CATEGORY_STYLES[(uint8_t)target];
}

// Grid: three rows of six 34px tiles
static const uint8_t SLOTS_PER_ROW = 6;
static const int16_t SLOT_SIZE = 34;
static const int16_t SLOT_PITCH = 38;
static const int16_t GRID_X = 8;
static const int16_t GRID_Y = 58;

// Gold card
static const int16_t CARD_X = 5;
static const int16_t CARD_Y = 31;
static const int16_t CARD_W = 230;
static const int16_t CARD_H = 22;
static const int16_t COIN_RADIUS = 5;

// Detail panel
static const int16_t PANEL_X = 5;
static const int16_t PANEL_Y = 174;
static const int16_t PANEL_W = 230;
static const int16_t PANEL_H = 132;
static const int16_t TEXT_X = PANEL_X + 7;
static const int16_t TEXT_RIGHT = PANEL_X + PANEL_W - 7;
static const int16_t CATEGORY_Y = PANEL_Y + 7;
static const int16_t TITLE_Y = PANEL_Y + 19;
static const int16_t LINE1_Y = PANEL_Y + 41;
static const int16_t LINE2_Y = PANEL_Y + 52;
static const int16_t DIVIDER_Y = PANEL_Y + 66;
static const int16_t EFFECT_Y = PANEL_Y + 74;
static const int16_t SAVINGS_Y = PANEL_Y + 90;
static const int16_t SAVINGS_H = 14;
static const int16_t BUY_ROW_Y = PANEL_Y + 116;
static const int16_t BADGE_OFFSET_Y = (8 - ButtonBadge::SIZE) / 2;

// Size 1 characters are 6px wide, so this many fit inside the detail panel
static const uint8_t DETAIL_LINE_CHARS = 35;

static const uint8_t NO_UPGRADE = 0xFF;

UpgradeScreen::UpgradeScreen(GameState &game, SoundManager &sound)
    : header("Upgrade", "Buildings", "Awards"),

      goldCard(CARD_X, CARD_Y, CARD_W, CARD_H, CARD_BORDER, false, 0x0000, true, 4),
      goldCoin(CARD_X + 12, CARD_Y + CARD_H / 2, COIN_RADIUS, COIN_RADIUS, GOLD_COLOR, true),
      goldText(CARD_X + 23, CARD_Y + 4, "0", GOLD_COLOR, 2),
      goldRate(CARD_X + CARD_W - 6, CARD_Y + 8, "+0 gold/s", RATE_COLOR, 1, TR_DATUM),

      detailPanel(PANEL_X, PANEL_Y, PANEL_W, PANEL_H, PANEL_BORDER, false, 0x0000, true, 4),
      categoryText(TEXT_X, CATEGORY_Y, "", 0xFFFF, 1),
      countText(TEXT_RIGHT, CATEGORY_Y, "", HINT_COLOR, 1, TR_DATUM),
      detailTitle(TEXT_X, TITLE_Y, "", 0xFFFF, 2),
      detailLine1(TEXT_X, LINE1_Y, "", LINE_COLOR, 1),
      detailLine2(TEXT_X, LINE2_Y, "", LINE_COLOR, 1),
      detailDivider(TEXT_X, DIVIDER_Y, TEXT_RIGHT - TEXT_X, 1, DIVIDER_COLOR, true, DIVIDER_COLOR),
      effectText(TEXT_X, EFFECT_Y, "", 0xFFFF, 1),
      savingsBar(TEXT_X, SAVINGS_Y, TEXT_RIGHT - TEXT_X, SAVINGS_H, 0, 100, SAVINGS_COLOR, SAVINGS_BG, SAVINGS_BORDER),
      buyBadge(TEXT_X, BUY_ROW_Y + BADGE_OFFSET_Y, ButtonBadge::CONFIRM),
      buyLabel(TEXT_X + ButtonBadge::SIZE + 4, BUY_ROW_Y, "Buy", 0xFFFF, 1),
      priceText(TEXT_RIGHT, BUY_ROW_Y, "", GOLD_COLOR, 1, TR_DATUM),

      game(game),
      sound(sound),
      lastRefresh(0),
      visibleCount(0),
      availableCount(0),
      selectedSlot(0),
      shownUpgrade(NO_UPGRADE)
{
    addElement(&header);
    setHeader(&header);

    addElement(&goldCard);
    addElement(&goldCoin);
    addElement(&goldText);
    addElement(&goldRate);

    for (uint8_t i = 0; i < MAX_VISIBLE_SLOTS; i++)
    {
        slots[i] = UpgradeSlot(GRID_X + (i % SLOTS_PER_ROW) * SLOT_PITCH, GRID_Y + (i / SLOTS_PER_ROW) * SLOT_PITCH,
                               SLOT_SIZE, SLOT_SIZE);
        visibleIds[i] = 0;
        addElement(&slots[i]);
    }

    addElement(&detailPanel);
    addElement(&categoryText);
    addElement(&countText);
    addElement(&detailTitle);
    addElement(&detailLine1);
    addElement(&detailLine2);
    addElement(&detailDivider);
    addElement(&effectText);
    addElement(&savingsBar);
    addElement(&buyBadge);
    addElement(&buyLabel);
    addElement(&priceText);

    rebuildVisible();
    refreshDetails();
}

void UpgradeScreen::rebuildVisible()
{
    visibleCount = 0;
    availableCount = 0;
    for (uint8_t i = 0; i < GOLD_UPGRADE_COUNT; i++)
    {
        if (game.isGoldUpgradeUnlocked(i) && !game.isGoldUpgradeBought(i))
        {
            availableCount++;
            if (visibleCount < MAX_VISIBLE_SLOTS)
            {
                visibleIds[visibleCount++] = i;
            }
        }
    }

    // A purchase shrinks the list, so the selection can end up past the end
    if (selectedSlot >= visibleCount)
    {
        selectedSlot = visibleCount > 0 ? visibleCount - 1 : 0;
    }
}

void UpgradeScreen::showDetailElements(bool visible)
{
    categoryText.setVisible(visible);
    detailDivider.setVisible(visible);
    effectText.setVisible(visible);
    savingsBar.setVisible(visible);
    buyBadge.setVisible(visible);
    buyLabel.setVisible(visible);
    priceText.setVisible(visible);
}

// What buying the upgrade changes, in the player's own numbers where the screen has them
void UpgradeScreen::formatEffect(const GoldUpgradeDef &def, char *buf, size_t size) const
{
    switch (def.target)
    {
    case UpgradeTarget::PRODUCTION:
    {
        uint64_t now = game.getProductionPerSecond();
        uint32_t bonus = game.getProductionBonusPercent();
        if (now == 0)
        {
            snprintf(buf, size, "Production bonus +%lu%% > +%lu%%", (unsigned long)bonus,
                     (unsigned long)(bonus + def.bonusPercent));
            return;
        }
        uint64_t after = now * (100 + bonus + def.bonusPercent) / (100 + bonus);
        snprintf(buf, size, "Income %s > %s", formatPerSecond(now).c_str(), formatPerSecond(after).c_str());
        return;
    }
    case UpgradeTarget::CLICK:
    {
        uint64_t base = ORE_TIERS[game.getOreTier()].clickAmount;
        uint64_t after = base * (100 + game.getClickBonusPercent() + def.bonusPercent) / 100;
        snprintf(buf, size, "Per click +%s > +%s", formatAmount(game.getClickAmount()).c_str(),
                 formatAmount(after).c_str());
        return;
    }
    case UpgradeTarget::ATTACK:
    case UpgradeTarget::DEFENSE:
    case UpgradeTarget::MAX_HP:
    {
        uint32_t now = def.target == UpgradeTarget::ATTACK    ? game.getAttackBonusPercent()
                       : def.target == UpgradeTarget::DEFENSE ? game.getDefenseBonusPercent()
                                                              : game.getMaxHpBonusPercent();
        const char *stat = def.target == UpgradeTarget::ATTACK    ? "Attack"
                           : def.target == UpgradeTarget::DEFENSE ? "Defense"
                                                                  : "Max HP";
        snprintf(buf, size, "%s bonus +%lu%% > +%lu%%", stat, (unsigned long)now,
                 (unsigned long)(now + def.bonusPercent));
        return;
    }
    case UpgradeTarget::UNLOCK_ZONE:
        // bonusPercent carries the zone index for this target
        snprintf(buf, size, "Opens zone: %s", def.bonusPercent < ZONE_COUNT ? ZONES[def.bonusPercent].name : "?");
        return;
    }
    buf[0] = '\0';
}

void UpgradeScreen::refreshDetails()
{
    char buf[48];

    snprintf(buf, sizeof(buf), "%u/%u bought", (unsigned)game.getUpgradesBoughtCount(), (unsigned)GOLD_UPGRADE_COUNT);
    if (availableCount > visibleCount)
    {
        snprintf(buf, sizeof(buf), "+%u more waiting", (unsigned)(availableCount - visibleCount));
    }
    countText.setText(buf);

    if (visibleCount == 0)
    {
        showDetailElements(false);
        shownUpgrade = NO_UPGRADE;
        detailTitle.setText("All caught up");
        detailLine1.setText("Buy buildings and mine deeper");
        detailLine2.setText("to unlock more upgrades.");
        return;
    }

    showDetailElements(true);

    uint8_t id = visibleIds[selectedSlot];
    const GoldUpgradeDef &def = GOLD_UPGRADES[id];
    const CategoryStyle &style = styleFor(def.target);

    // Name, category and description are fixed per upgrade, so only rewritten when the
    // selection moves to a different one
    if (shownUpgrade != id)
    {
        shownUpgrade = id;
        categoryText.setText(style.label);
        categoryText.setColor(style.accent);
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
    }

    formatEffect(def, buf, sizeof(buf));
    effectText.setText(buf);

    // Savings toward the price, so a far-off upgrade still shows how close it is
    uint64_t gold = game.getGold();
    bool affordable = game.canAffordGoldUpgrade(id);
    uint32_t percent = (affordable || def.cost == 0) ? 100 : (uint32_t)(gold * 100 / def.cost);
    savingsBar.setProgress(percent, 100);
    savingsBar.setFillColor(affordable ? READY_FILL : SAVINGS_COLOR);
    if (affordable)
    {
        savingsBar.setLabel("Ready to buy");
    }
    else
    {
        snprintf(buf, sizeof(buf), "%s / %s  (%lu%%)", formatAmount(gold).c_str(), formatAmount(def.cost).c_str(),
                 (unsigned long)percent);
        savingsBar.setLabel(buf);
    }

    buyBadge.setDimmed(!affordable);
    snprintf(buf, sizeof(buf), "%s gold", formatAmount(def.cost).c_str());
    priceText.setText(buf);
    priceText.setColor(affordable ? AFFORD_COLOR : HINT_COLOR);
}

void UpgradeScreen::update(unsigned long now)
{
    if (lastRefresh != 0 && now - lastRefresh < REFRESH_MS)
    {
        return;
    }
    lastRefresh = now;

    goldText.setText(formatAmount(game.getGold()));
    goldRate.setText(formatRate(game.getProductionPerSecond()));

    rebuildVisible();

    for (uint8_t i = 0; i < MAX_VISIBLE_SLOTS; i++)
    {
        UpgradeSlot &slot = slots[i];
        bool used = i < visibleCount;
        slot.setVisible(used);
        if (!used)
        {
            continue;
        }

        uint8_t id = visibleIds[i];
        const CategoryStyle &style = styleFor(GOLD_UPGRADES[id].target);
        slot.setIcon(style.icon);
        slot.setAccent(style.accent);

        if (i == selectedSlot)
        {
            slot.setState(UpgradeSlot::SELECTED);
        }
        else
        {
            slot.setState(game.canAffordGoldUpgrade(id) ? UpgradeSlot::AFFORDABLE : UpgradeSlot::TOO_DEAR);
        }
    }

    refreshDetails();
}

void UpgradeScreen::onUpPress()
{
    if (visibleCount > 0)
    {
        selectedSlot = (selectedSlot == 0) ? (visibleCount - 1) : (selectedSlot - 1);
    }
    lastRefresh = 0;
}

void UpgradeScreen::onDownPress()
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
