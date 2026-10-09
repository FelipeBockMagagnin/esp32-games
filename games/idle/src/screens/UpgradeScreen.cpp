#include "UpgradeScreen.h"
#include "../game/Format.h"
#include "../game/CombatConfig.h" // ZONES, for the zone an unlock opens


static const uint16_t CARD_BORDER = Theme::GOLD_FRAME; // Dark amber, same frame as the other gold cards
static const uint16_t RATE_COLOR = Theme::GOOD;  // Soft green: income that grows on its own
static const uint16_t PANEL_BORDER = Theme::FRAME; // Dark slate
static const uint16_t DIVIDER_COLOR = Theme::FRAME;
static const uint16_t LINE_COLOR = Theme::TEXT_2;  // Description, a step below the white title
static const uint16_t HINT_COLOR = Theme::TEXT_2;
static const uint16_t AFFORD_COLOR = Theme::GOOD;
static const uint16_t SAVINGS_COLOR = Theme::GOLD_BAR; // Amber fill, dark enough for the white label
static const uint16_t SAVINGS_BG = Theme::GOLD_TRACK;
static const uint16_t SAVINGS_BORDER = Theme::GOLD_BORDER;
static const uint16_t READY_FILL = Theme::DONE_BAR; // Dark green once the price is covered

// How each upgrade category looks, indexed by UpgradeTarget. The colors echo where the
// bonus lands: gold for production, the combat screen's red/blue/green for its stats.
struct CategoryStyle
{
    const char *label;
    uint16_t accent;
    const uint16_t *icon;
};

static const CategoryStyle CATEGORY_STYLES[] = {
    {"PRODUCTION", Theme::GOLD, image_upgrade_production_pixels}, // PRODUCTION
    {"MINING", Theme::GOLD_BAR, image_upgrade_click_pixels},    // CLICK
    {"ATTACK", Theme::ATTACK, image_sword_02b_pixels},          // ATTACK
    {"DEFENSE", Theme::DEFENSE, image_armor_01b_pixels},        // DEFENSE
    {"HEALTH", Theme::HP, image_upgrade_heart_pixels},          // MAX_HP
    {"NEW ZONE", Theme::EPIC, image_upgrade_map_pixels},        // UNLOCK_ZONE
    {"MINING", Theme::GOLD_BAR, image_upgrade_click_pixels},    // CLICK_PRODUCTION
    {"BUILDING", Theme::INFO, image_upgrade_production_pixels}, // BUILDING_BOOST, icon per building
    {"GOLD VEIN", Theme::GOLD, image_icon_gold_vein_pixels},    // VEIN_FREQUENCY
    {"GOLD VEIN", Theme::GOLD, image_icon_gold_vein_pixels},    // VEIN_POWER
};

static const CategoryStyle &styleFor(UpgradeTarget target)
{
    return CATEGORY_STYLES[(uint8_t)target];
}

// A building boost shows the building it boosts rather than the category's icon
static const uint16_t *iconFor(const GoldUpgradeDef &def)
{
    if (def.target == UpgradeTarget::BUILDING_BOOST && def.reqBuilding < BUILDING_COUNT)
    {
        return BUILDINGS[def.reqBuilding].icon;
    }
    return styleFor(def.target).icon;
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
    : header("Upgrade"),

      goldCard(CARD_X, CARD_Y, CARD_W, CARD_H, CARD_BORDER, true, Theme::CARD, true, 4),
      goldCoin(CARD_X + 12, CARD_Y + CARD_H / 2, COIN_RADIUS, COIN_RADIUS, Theme::GOLD, true),
      goldText(CARD_X + 23, CARD_Y + 4, "0", Theme::GOLD, 2),
      goldRate(CARD_X + CARD_W - 6, CARD_Y + 8, "+0 gold/s", RATE_COLOR, 1, TR_DATUM),

      detailPanel(PANEL_X, PANEL_Y, PANEL_W, PANEL_H, PANEL_BORDER, true, Theme::CARD, true, 4),
      categoryText(TEXT_X, CATEGORY_Y, "", Theme::TEXT, 1),
      countText(TEXT_RIGHT, CATEGORY_Y, "", HINT_COLOR, 1, TR_DATUM),
      detailTitle(TEXT_X, TITLE_Y, "", Theme::TEXT, 2),
      detailLine1(TEXT_X, LINE1_Y, "", LINE_COLOR, 1),
      detailLine2(TEXT_X, LINE2_Y, "", LINE_COLOR, 1),
      detailDivider(TEXT_X, DIVIDER_Y, TEXT_RIGHT - TEXT_X, 1, DIVIDER_COLOR, true, DIVIDER_COLOR),
      effectText(TEXT_X, EFFECT_Y, "", Theme::TEXT, 1),
      savingsBar(TEXT_X, SAVINGS_Y, TEXT_RIGHT - TEXT_X, SAVINGS_H, 0, 100, SAVINGS_COLOR, SAVINGS_BG, SAVINGS_BORDER),
      buyBadge(TEXT_X, BUY_ROW_Y + BADGE_OFFSET_Y, ButtonBadge::CONFIRM),
      buyLabel(TEXT_X + ButtonBadge::SIZE + 4, BUY_ROW_Y, "Buy", Theme::TEXT, 1),
      priceText(TEXT_RIGHT, BUY_ROW_Y, "", Theme::GOLD, 1, TR_DATUM),

      game(game),
      sound(sound),
      visibleCount(0),
      availableCount(0),
      selectedSlot(0),
      shownUpgrade(NO_UPGRADE)
{
    addElement(&header);
    setHeader(&header);

    addElement(&goldCard);
    addElement(&goldCoin, Theme::CARD);
    addElement(&goldText, Theme::CARD);
    addElement(&goldRate, Theme::CARD);

    for (uint8_t i = 0; i < MAX_VISIBLE_SLOTS; i++)
    {
        slots[i] = UpgradeSlot(GRID_X + (i % SLOTS_PER_ROW) * SLOT_PITCH, GRID_Y + (i / SLOTS_PER_ROW) * SLOT_PITCH,
                               SLOT_SIZE, SLOT_SIZE);
        visibleIds[i] = 0;
        addElement(&slots[i]);
    }

    addElement(&detailPanel);
    addElement(&categoryText, Theme::CARD);
    addElement(&countText, Theme::CARD);
    addElement(&detailTitle, Theme::CARD);
    addElement(&detailLine1, Theme::CARD);
    addElement(&detailLine2, Theme::CARD);
    addElement(&detailDivider, Theme::CARD);
    addElement(&effectText, Theme::CARD);
    addElement(&savingsBar, Theme::CARD);
    addElement(&buyBadge, Theme::CARD);
    addElement(&buyLabel, Theme::CARD);
    addElement(&priceText, Theme::CARD);

    rebuildVisible();
    refreshDetails();
}

// The cheapest available upgrades, cheapest first, so the grid reads as a shopping list:
// the first tile is always the next thing the player can afford. Kept as an insertion into
// a sorted list of at most MAX_VISIBLE_SLOTS, so the pricier ones simply fall off the end.
void UpgradeScreen::rebuildVisible()
{
    visibleCount = 0;
    availableCount = 0;
    for (uint8_t i = 0; i < GOLD_UPGRADE_COUNT; i++)
    {
        if (!game.isGoldUpgradeUnlocked(i) || game.isGoldUpgradeBought(i))
        {
            continue;
        }
        availableCount++;

        uint8_t pos = visibleCount;
        while (pos > 0 && GOLD_UPGRADES[visibleIds[pos - 1]].cost > GOLD_UPGRADES[i].cost)
        {
            pos--;
        }
        if (pos >= MAX_VISIBLE_SLOTS)
        {
            continue;
        }
        uint8_t last = visibleCount < MAX_VISIBLE_SLOTS ? visibleCount : MAX_VISIBLE_SLOTS - 1;
        for (uint8_t k = last; k > pos; k--)
        {
            visibleIds[k] = visibleIds[k - 1];
        }
        visibleIds[pos] = i;
        if (visibleCount < MAX_VISIBLE_SLOTS)
        {
            visibleCount++;
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
    case UpgradeTarget::CLICK_PRODUCTION:
    {
        // Both add to what a click already earns: the ore's base value scaled up, or a
        // share of production per second
        uint64_t added = def.target == UpgradeTarget::CLICK
                             ? (uint64_t)ORE_TIERS[game.getOreTier()].clickAmount * def.bonusPercent / 100
                             : game.getProductionPerSecond() * def.bonusPercent / 100 / GOLD_SCALE;
        uint64_t now = game.getClickAmount();
        snprintf(buf, size, "Per click +%s > +%s", formatAmount(now).c_str(), formatAmount(now + added).c_str());
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
    case UpgradeTarget::VEIN_FREQUENCY:
    {
        // The base range shortened by the frequency bonus, before and after
        uint32_t now = 100 + game.getVeinFrequencyPercent();
        uint32_t after = now + def.bonusPercent;
        snprintf(buf, size, "Every %lu-%lus > %lu-%lus", VEIN_MIN_INTERVAL_MS / 10 / now, VEIN_MAX_INTERVAL_MS / 10 / now,
                 VEIN_MIN_INTERVAL_MS / 10 / after, VEIN_MAX_INTERVAL_MS / 10 / after);
        return;
    }
    case UpgradeTarget::VEIN_POWER:
    {
        uint32_t now = 100 + game.getVeinPowerPercent();
        snprintf(buf, size, "Frenzy %lus > %lus", VEIN_FRENZY_MS * now / 100000,
                 VEIN_FRENZY_MS * (now + def.bonusPercent) / 100000);
        return;
    }
    case UpgradeTarget::BUILDING_BOOST:
    {
        uint64_t now = game.getBuildingProduction(def.reqBuilding);
        snprintf(buf, size, "%s %s > %s", BUILDINGS[def.reqBuilding].name, formatPerSecond(now).c_str(),
                 formatPerSecond(now * 2).c_str());
        return;
    }
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
    uint64_t cost = game.getGoldUpgradeCost(id);
    uint32_t percent = (affordable || cost == 0) ? 100 : (uint32_t)(gold * 100 / cost);
    savingsBar.setProgress(percent, 100);
    savingsBar.setFillColor(affordable ? READY_FILL : SAVINGS_COLOR);
    if (affordable)
    {
        savingsBar.setLabel("Ready to buy");
    }
    else
    {
        snprintf(buf, sizeof(buf), "%s / %s  (%lu%%)", formatAmount(gold).c_str(), formatAmount(cost).c_str(),
                 (unsigned long)percent);
        savingsBar.setLabel(buf);
    }

    buyBadge.setDimmed(!affordable);
    snprintf(buf, sizeof(buf), "%s gold", formatAmount(cost).c_str());
    priceText.setText(buf);
    priceText.setColor(affordable ? AFFORD_COLOR : HINT_COLOR);
}

void UpgradeScreen::update(unsigned long now)
{
    if (!refreshDue(now))
    {
        return;
    }

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
        slot.setIcon(iconFor(GOLD_UPGRADES[id]));
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
    forceRefresh();
}

void UpgradeScreen::onDownPress()
{
    if (visibleCount > 0)
    {
        selectedSlot = (selectedSlot + 1) % visibleCount;
    }
    forceRefresh();
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
    forceRefresh();
}
