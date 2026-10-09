#include "InventoryScreen.h"
#include "../game/CombatConfig.h" // spellForItem


static const uint16_t CARD_BORDER = Theme::FRAME;   // Dark slate: the player's side, not the gold cards
static const uint16_t PANEL_BORDER = Theme::FRAME;
static const uint16_t ATTACK_COLOR = Theme::ATTACK;  // Soft red, as on the combat screen
static const uint16_t DEFENSE_COLOR = Theme::DEFENSE; // Steel blue
static const uint16_t HP_COLOR = Theme::HP;      // Leaf green
static const uint16_t HINT_COLOR = Theme::TEXT_2;
static const uint16_t VALUE_COLOR = Theme::TEXT;   // An item's own stats, neither better nor worse
static const uint16_t BETTER_COLOR = Theme::GOOD;
static const uint16_t WORSE_COLOR = Theme::BAD;
static const uint16_t SAME_COLOR = Theme::TEXT_DIM;

// Stats card
static const int16_t CARD_X = 5;
static const int16_t CARD_Y = 31;
static const int16_t CARD_W = 230;
static const int16_t CARD_H = 30;
static const int16_t STAT_ICON_Y = CARD_Y + 7;
static const int16_t STAT_TEXT_Y = CARD_Y + 8;

// List
static const int16_t ROW_X = 5;
static const int16_t LIST_TOP = 65;
static const int16_t ROW_PITCH = 28;

// Detail panel; text (8px) and badge (11px) are centred on the row's middle
static const int16_t PANEL_X = 5;
static const int16_t PANEL_Y = 263;
static const int16_t PANEL_W = 230;
static const int16_t PANEL_H = 54;
static const int16_t TEXT_X = PANEL_X + 7;
static const int16_t TEXT_RIGHT = PANEL_X + PANEL_W - 7;
static const int16_t TITLE_Y = PANEL_Y + 6;
static const int16_t STATS_Y = PANEL_Y + 21;
static const int16_t HINT_ROW_Y = PANEL_Y + 38;
static const int16_t BADGE_OFFSET_Y = (8 - ButtonBadge::SIZE) / 2;
// Four columns: "ATK +135" needs 48px, the gold column ("CLK +100%") a little more
static const int16_t STAT_COLUMN_X[] = {TEXT_X, TEXT_X + 52, TEXT_X + 104, TEXT_X + 156};

// Compact "atk 12 def 4 hp 30 +15% click", skipping whatever the item does not give
static String describeItem(const Inventory &inventory, uint8_t itemId)
{
    Stats s = inventory.getItemStats(itemId);
    const ItemDef &def = ITEMS[itemId];
    uint8_t level = inventory.getItemLevel(itemId);
    char buf[48];
    size_t n = 0;
    buf[0] = '\0';

    if (s.attack > 0)
    {
        n += snprintf(buf + n, sizeof(buf) - n, "atk %u", (unsigned)s.attack);
    }
    if (s.defense > 0 && n < sizeof(buf) - 1)
    {
        n += snprintf(buf + n, sizeof(buf) - n, "%sdef %u", n > 0 ? " " : "", (unsigned)s.defense);
    }
    if (s.maxHp > 0 && n < sizeof(buf) - 1)
    {
        n += snprintf(buf + n, sizeof(buf) - n, "%shp %u", n > 0 ? " " : "", (unsigned)s.maxHp);
    }
    if (def.goldProductionBonusPercent > 0 && n < sizeof(buf) - 1)
    {
        n += snprintf(buf + n, sizeof(buf) - n, "%s+%lu%% gold", n > 0 ? " " : "",
                      (unsigned long)scaleItemStat(def.goldProductionBonusPercent, level));
    }
    if (def.clickBonusPercent > 0 && n < sizeof(buf) - 1)
    {
        snprintf(buf + n, sizeof(buf) - n, "%s+%lu%% click", n > 0 ? " " : "",
                 (unsigned long)scaleItemStat(def.clickBonusPercent, level));
    }
    return String(buf);
}

// How many items exist for a slot, for the "2/4 found" counts
static uint8_t itemsInSlot(EquipSlot slot)
{
    uint8_t count = 0;
    for (uint8_t i = 0; i < ITEM_COUNT; i++)
    {
        if (ITEMS[i].slot == slot)
        {
            count++;
        }
    }
    return count;
}

// The slot's first item, whose icon stands in (grayed) for an empty slot
static const ItemDef *firstItemOf(EquipSlot slot)
{
    for (uint8_t i = 0; i < ITEM_COUNT; i++)
    {
        if (ITEMS[i].slot == slot)
        {
            return &ITEMS[i];
        }
    }
    return nullptr;
}

static void setRowIcon(ListRow &row, const ItemDef &def)
{
    if (def.iconPixels != nullptr)
    {
        row.setIconPixels(def.iconPixels, def.iconW, def.iconH);
    }
    else
    {
        row.setIconBitmap(def.iconBitmap, def.iconW, def.iconH);
    }
}

InventoryScreen::InventoryScreen(GameState &game, Inventory &inventory, SoundManager &sound)
    : header("Inventory"),

      statsCard(CARD_X, CARD_Y, CARD_W, CARD_H, CARD_BORDER, true, Theme::CARD, true, 4),
      attackIcon(CARD_X + 8, STAT_ICON_Y, 16, 16, image_sword_02b_pixels, true, 0x0000),
      attackText(CARD_X + 28, STAT_TEXT_Y, "0", ATTACK_COLOR, 2),
      defenseIcon(CARD_X + 84, STAT_ICON_Y, 16, 16, image_armor_01b_pixels, true, 0x0000),
      defenseText(CARD_X + 104, STAT_TEXT_Y, "0", DEFENSE_COLOR, 2),
      hpIcon(CARD_X + 158, STAT_ICON_Y, 16, 16, image_upgrade_heart_pixels, true, 0x0000),
      hpText(CARD_X + 178, STAT_TEXT_Y, "0", HP_COLOR, 2),

      list(ROWS),

      detailPanel(PANEL_X, PANEL_Y, PANEL_W, PANEL_H, PANEL_BORDER, true, Theme::CARD, true, 4),
      panelTitle(TEXT_X, TITLE_Y, "", Theme::TEXT, 1),
      panelRight(TEXT_RIGHT, TITLE_Y, "", HINT_COLOR, 1, TR_DATUM),
      actionBadge(TEXT_X, HINT_ROW_Y + BADGE_OFFSET_Y, ButtonBadge::CONFIRM),
      actionLabel(TEXT_X + ButtonBadge::SIZE + 4, HINT_ROW_Y, "", Theme::TEXT, 1),
      hintText(TEXT_RIGHT, HINT_ROW_Y, "", HINT_COLOR, 1, TR_DATUM),

      game(game),
      inventory(inventory),
      sound(sound),
      mode(SLOT_LIST),
      inspecting(EquipSlot::HELMET),
      lastRevision(0),
      rowsDirty(true)
{
    addElement(&header);
    setHeader(&header);

    addElement(&statsCard);
    addElement(&attackIcon, Theme::CARD);
    addElement(&attackText, Theme::CARD);
    addElement(&defenseIcon, Theme::CARD);
    addElement(&defenseText, Theme::CARD);
    addElement(&hpIcon, Theme::CARD);
    addElement(&hpText, Theme::CARD);

    for (uint8_t i = 0; i < ROWS; i++)
    {
        rows[i].setPosition(ROW_X, LIST_TOP + i * ROW_PITCH);
        addElement(&rows[i]);
    }

    addElement(&detailPanel);
    addElement(&panelTitle, Theme::CARD);
    addElement(&panelRight, Theme::CARD);
    for (uint8_t i = 0; i < STAT_COLUMNS; i++)
    {
        statColumns[i] = Text(STAT_COLUMN_X[i], STATS_Y, "", VALUE_COLOR, 1);
        addElement(&statColumns[i], Theme::CARD);
    }
    addElement(&actionBadge, Theme::CARD);
    addElement(&actionLabel, Theme::CARD);
    addElement(&hintText, Theme::CARD);

    enterSlotList();
}

void InventoryScreen::onEnter(TFT_eSPI &tft)
{
    // Always come back to the slot list rather than a half-finished equip
    enterSlotList();
    Screen::onEnter(tft);
}

void InventoryScreen::enterSlotList()
{
    mode = SLOT_LIST;
    header.setTitle("Inventory");
    list.setCount(EQUIP_SLOT_COUNT);
    list.reset();
    rowsDirty = true;
    forceRefresh();
}

void InventoryScreen::enterItemList(EquipSlot slot)
{
    mode = ITEM_LIST;
    inspecting = slot;
    header.setTitle(EQUIP_SLOT_NAMES[(uint8_t)slot]);
    // Row 0 is the way back out; with two buttons there is nowhere else to put it
    list.setCount(1 + inventory.countItemsForSlot(slot));
    list.reset();
    rowsDirty = true;
    forceRefresh();
}

void InventoryScreen::refreshRows()
{
    uint8_t visible = list.getVisibleCount();

    for (uint8_t i = 0; i < ROWS; i++)
    {
        ListRow &row = rows[i];
        if (i >= visible)
        {
            row.setVisible(false);
            continue;
        }

        uint16_t index = list.getFirstVisible() + i;
        bool isSelected = index == list.getSelected();
        row.setVisible(true);

        if (mode == SLOT_LIST)
        {
            EquipSlot slot = (EquipSlot)index;
            int8_t itemId = inventory.getEquipped(slot);

            if (itemId == NO_ITEM)
            {
                // The slot's first item, grayed out, shows what goes here
                row.setTitle(EQUIP_SLOT_NAMES[index]);
                row.setSubtitle(inventory.countItemsForSlot(slot) > 0 ? "empty, items to wear" : "empty");
                row.setValue("");
                row.setValueSub("");
                const ItemDef *placeholder = firstItemOf(slot);
                if (placeholder != nullptr)
                {
                    setRowIcon(row, *placeholder);
                }
                row.setTitleColor(Theme::TEXT);
                row.setState(isSelected ? ListRow::SELECTED : ListRow::DIMMED);
            }
            else
            {
                const ItemDef &def = ITEMS[itemId];
                char buf[12];
                row.setTitle(def.name);
                row.setTitleColor(ITEM_TIER_COLORS[def.tier]);
                row.setSubtitle(describeItem(inventory, (uint8_t)itemId));
                snprintf(buf, sizeof(buf), "Lv %u", (unsigned)inventory.getItemLevel((uint8_t)itemId));
                row.setValue(buf);
                row.setValueSub(EQUIP_SLOT_NAMES[index]);
                setRowIcon(row, def);
                row.setState(isSelected ? ListRow::SELECTED : ListRow::NORMAL);
            }
            continue;
        }

        if (index == 0)
        {
            row.setTitle("< Back");
            row.setTitleColor(Theme::TEXT);
            row.setSubtitle("to all slots");
            row.setValue("");
            row.setValueSub("");
            row.clearIcon();
            row.setState(isSelected ? ListRow::SELECTED : ListRow::NORMAL);
            continue;
        }

        int8_t itemId = inventory.itemForSlotAt(inspecting, (uint8_t)(index - 1));
        if (itemId == NO_ITEM)
        {
            row.setVisible(false);
            continue;
        }

        const ItemDef &def = ITEMS[itemId];
        bool isEquipped = inventory.getEquipped(inspecting) == itemId;
        char buf[12];

        row.setTitle(def.name);
        row.setTitleColor(ITEM_TIER_COLORS[def.tier]);
        row.setSubtitle(describeItem(inventory, (uint8_t)itemId));
        snprintf(buf, sizeof(buf), "Lv %u", (unsigned)inventory.getItemLevel((uint8_t)itemId));
        row.setValue(buf);
        row.setValueSub(isEquipped ? "worn" : ITEM_TIER_NAMES[def.tier]);
        setRowIcon(row, def);

        if (isSelected)
        {
            row.setState(ListRow::SELECTED);
        }
        else
        {
            row.setState(isEquipped ? ListRow::OWNED : ListRow::NORMAL);
        }
    }
}

void InventoryScreen::refreshTotals()
{
    Stats total = resolvePlayerStats(game, inventory);
    char buf[12];

    snprintf(buf, sizeof(buf), "%u", (unsigned)total.attack);
    attackText.setText(buf);
    snprintf(buf, sizeof(buf), "%u", (unsigned)total.defense);
    defenseText.setText(buf);
    snprintf(buf, sizeof(buf), "%u", (unsigned)total.maxHp);
    hpText.setText(buf);
}

void InventoryScreen::hideStats()
{
    for (uint8_t i = 0; i < STAT_COLUMNS; i++)
    {
        statColumns[i].setVisible(false);
    }
}

// What a stat column shows. Columns 2 and 3 change kind with the slot: rings and amulets
// give no HP, so their respawn cut takes that column, and the last one holds whichever
// extra the slot gives (clicks for rings, production for amulets, regen for armour).
enum StatKind : uint8_t
{
    STAT_ATTACK,
    STAT_DEFENSE,
    STAT_HP,
    STAT_RESPAWN,
    STAT_CLICK,
    STAT_GOLD,
    STAT_REGEN
};

static int32_t statValue(const Inventory &inventory, uint8_t id, StatKind kind)
{
    Stats s = inventory.getItemStats(id);
    uint8_t level = inventory.getItemLevel(id);
    switch (kind)
    {
    case STAT_ATTACK:
        return (int32_t)s.attack;
    case STAT_DEFENSE:
        return (int32_t)s.defense;
    case STAT_HP:
        return (int32_t)s.maxHp;
    case STAT_RESPAWN:
        return -(int32_t)ITEMS[id].respawnCutPercent;
    case STAT_CLICK:
        return (int32_t)scaleItemStat(ITEMS[id].clickBonusPercent, level);
    case STAT_GOLD:
        return (int32_t)scaleItemStat(ITEMS[id].goldProductionBonusPercent, level);
    case STAT_REGEN:
        return (int32_t)s.regen;
    }
    return 0;
}

// Both sides of a comparison share a slot, so either one having the stat picks the kind
static bool eitherHas(int8_t a, int8_t b, uint16_t ItemDef::*field)
{
    return (a != NO_ITEM && ITEMS[a].*field > 0) || (b != NO_ITEM && ITEMS[b].*field > 0);
}

// A relic's one effect, in the first column. Two relics do different things, so a relic is
// described on its own rather than compared with the one worn.
static void formatRelic(const Inventory &inventory, uint8_t id, char *buf, size_t size)
{
    uint32_t value = scaleItemStat(ITEMS[id].relicPercent, inventory.getItemLevel(id));
    switch (ITEMS[id].relic)
    {
    case RelicEffect::UPGRADE_DISCOUNT:
        snprintf(buf, size, "Upgrades -%lu%%", (unsigned long)value);
        return;
    case RelicEffect::COOLDOWN_CUT:
        snprintf(buf, size, "Strike/guard cooldown -%lu%%", (unsigned long)value);
        return;
    case RelicEffect::AWARD_BOOST:
        snprintf(buf, size, "Award gold bonus +%lu%%", (unsigned long)value);
        return;
    case RelicEffect::HP_REGEN:
        snprintf(buf, size, "Regen %lu.%lu%% max HP/s", (unsigned long)(value / 10), (unsigned long)(value % 10));
        return;
    default:
        buf[0] = '\0';
        return;
    }
}

void InventoryScreen::showStats(int8_t itemId, int8_t against, bool compare)
{
    if (itemId != NO_ITEM && ITEMS[itemId].slot == EquipSlot::RELIC)
    {
        char relicBuf[36];
        formatRelic(inventory, (uint8_t)itemId, relicBuf, sizeof(relicBuf));
        statColumns[0].setText(relicBuf);
        statColumns[0].setColor(ITEM_TIER_COLORS[RELIC_TIER]);
        statColumns[0].setVisible(true);
        for (uint8_t i = 1; i < STAT_COLUMNS; i++)
        {
            statColumns[i].setVisible(false);
        }
        return;
    }

    StatKind kinds[STAT_COLUMNS] = {STAT_ATTACK, STAT_DEFENSE, STAT_HP, STAT_GOLD};
    if ((itemId != NO_ITEM && ITEMS[itemId].respawnCutPercent > 0) ||
        (against != NO_ITEM && ITEMS[against].respawnCutPercent > 0))
    {
        kinds[2] = STAT_RESPAWN;
    }
    if (eitherHas(itemId, against, &ItemDef::clickBonusPercent))
    {
        kinds[3] = STAT_CLICK;
    }
    else if (eitherHas(itemId, against, &ItemDef::regen))
    {
        kinds[3] = STAT_REGEN;
    }

    static const char *const LABELS[] = {"ATK", "DEF", "HP", "RSP", "CLK", "GLD", "RGN"};
    char buf[16];
    for (uint8_t i = 0; i < STAT_COLUMNS; i++)
    {
        StatKind kind = kinds[i];
        int32_t mine = itemId != NO_ITEM ? statValue(inventory, (uint8_t)itemId, kind) : 0;
        int32_t theirs = against != NO_ITEM ? statValue(inventory, (uint8_t)against, kind) : 0;
        int32_t value = compare ? mine - theirs : mine;

        Text &column = statColumns[i];
        bool shown = compare ? (mine != 0 || theirs != 0) : mine != 0;
        column.setVisible(shown);
        if (!shown)
        {
            continue;
        }

        bool percent = kind == STAT_RESPAWN || kind == STAT_CLICK || kind == STAT_GOLD;
        const char *sign = value > 0 && (compare || percent || kind == STAT_REGEN) ? "+" : value < 0 ? "-" : "";
        unsigned long magnitude = (unsigned long)(value < 0 ? -value : value);
        if (kind == STAT_REGEN)
        {
            snprintf(buf, sizeof(buf), "%s %s%lu.%lu", LABELS[kind], sign, magnitude / 10, magnitude % 10);
        }
        else
        {
            snprintf(buf, sizeof(buf), "%s %s%lu%s", LABELS[kind], sign, magnitude, percent ? "%" : "");
        }
        column.setText(buf);

        // A respawn cut is stored negative, so less is better there
        int32_t gain = kind == STAT_RESPAWN ? -value : value;
        column.setColor(!compare ? VALUE_COLOR : gain > 0 ? BETTER_COLOR : gain < 0 ? WORSE_COLOR : SAME_COLOR);
    }
}

void InventoryScreen::refreshPanel()
{
    char buf[24];

    if (mode == SLOT_LIST)
    {
        EquipSlot slot = (EquipSlot)list.getSelected();
        int8_t equipped = inventory.getEquipped(slot);
        uint8_t found = inventory.countItemsForSlot(slot);

        panelTitle.setText(EQUIP_SLOT_NAMES[(uint8_t)slot]);
        panelTitle.setColor(Theme::TEXT);
        snprintf(buf, sizeof(buf), "%u/%u found", (unsigned)found, (unsigned)itemsInSlot(slot));
        panelRight.setText(buf);

        if (equipped != NO_ITEM)
        {
            showStats(equipped, NO_ITEM, false);
        }
        else
        {
            hideStats();
        }

        actionBadge.setDimmed(found == 0);
        actionLabel.setText(found == 0 ? "Nothing found yet" : "Choose item");
        actionLabel.setColor(found == 0 ? HINT_COLOR : Theme::TEXT);
        snprintf(buf, sizeof(buf), "%u/%u items", (unsigned)inventory.countOwnedItems(), (unsigned)ITEM_COUNT);
        hintText.setText(buf);
        hintText.setColor(HINT_COLOR);
        return;
    }

    hintText.setText("Back: slots");
    hintText.setColor(HINT_COLOR);

    uint16_t selected = list.getSelected();
    int8_t itemId = selected == 0 ? NO_ITEM : inventory.itemForSlotAt(inspecting, (uint8_t)(selected - 1));
    if (itemId == NO_ITEM)
    {
        panelTitle.setText("Back to slots");
        panelTitle.setColor(Theme::TEXT);
        panelRight.setText("");
        hideStats();
        actionBadge.setDimmed(false);
        actionLabel.setText("Back");
        actionLabel.setColor(Theme::TEXT);
        return;
    }

    const ItemDef &def = ITEMS[itemId];
    int8_t equipped = inventory.getEquipped(inspecting);
    bool isEquipped = equipped == itemId;

    panelTitle.setText(def.name);
    panelTitle.setColor(ITEM_TIER_COLORS[def.tier]);
    snprintf(buf, sizeof(buf), "%s  Lv %u", ITEM_TIER_NAMES[def.tier], (unsigned)inventory.getItemLevel((uint8_t)itemId));
    panelRight.setText(buf);

    // Against the worn item (or against nothing, which shows the full gain), so a
    // sidegrade's trade-off is visible before committing to it
    showStats(itemId, isEquipped ? NO_ITEM : equipped, !isEquipped);

    // An amulet's real worth is the spell it puts on the Up button in combat
    const SpellDef *spell = spellForItem(itemId);
    if (spell != nullptr)
    {
        snprintf(buf, sizeof(buf), "Spell: %s", spell->name);
        hintText.setText(buf);
        hintText.setColor(Theme::EPIC);
    }

    actionBadge.setDimmed(isEquipped);
    actionLabel.setText(isEquipped ? "Already worn" : "Equip");
    actionLabel.setColor(isEquipped ? HINT_COLOR : Theme::TEXT);
}

void InventoryScreen::update(unsigned long now)
{
    // A drop landing while this screen is open changes the list under the player
    uint32_t revision = inventory.getRevision();
    if (revision != lastRevision)
    {
        lastRevision = revision;
        if (mode == ITEM_LIST)
        {
            list.setCount(1 + inventory.countItemsForSlot(inspecting));
        }
        rowsDirty = true;
        forceRefresh();
    }

    if (!refreshDue(now))
    {
        return;
    }

    if (rowsDirty)
    {
        rowsDirty = false;
        refreshRows();
        refreshPanel();
    }
    // Upgrades bought elsewhere change the totals without touching the inventory
    refreshTotals();
}

void InventoryScreen::onUpPress()
{
    list.previous();
    rowsDirty = true;
    forceRefresh();
}

void InventoryScreen::onDownPress()
{
    list.next();
    rowsDirty = true;
    forceRefresh();
}

void InventoryScreen::onConfirmPress()
{
    if (mode == SLOT_LIST)
    {
        EquipSlot slot = (EquipSlot)list.getSelected();
        if (inventory.countItemsForSlot(slot) == 0)
        {
            sound.playError();
            return;
        }
        sound.playMenu();
        enterItemList(slot);
        return;
    }

    uint16_t selected = list.getSelected();
    if (selected == 0)
    {
        sound.playMenu();
        enterSlotList();
        return;
    }

    int8_t itemId = inventory.itemForSlotAt(inspecting, (uint8_t)(selected - 1));
    if (itemId != NO_ITEM && inventory.equip((uint8_t)itemId))
    {
        sound.playBuy();
    }
    else
    {
        sound.playError();
    }
    enterSlotList();
}

bool InventoryScreen::onBackPress()
{
    if (mode == ITEM_LIST)
    {
        sound.playMenu();
        enterSlotList();
        return true;
    }
    return false;
}
