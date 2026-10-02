#include "InventoryScreen.h"

static const unsigned long REFRESH_MS = 100;

static const uint16_t CARD_BORDER = 0x31A6;   // Dark slate: the player's side, not the gold cards
static const uint16_t PANEL_BORDER = 0x31A6;
static const uint16_t ROW_FRAME = 0x31A6;
static const uint16_t SELECTED_FILL = 0x2124; // Faint highlight behind the selected row
static const uint16_t ATTACK_COLOR = 0xFB2C;  // Soft red, as on the combat screen
static const uint16_t DEFENSE_COLOR = 0x6E7F; // Steel blue
static const uint16_t HP_COLOR = 0x2D45;      // Leaf green
static const uint16_t HINT_COLOR = 0x94B2;
static const uint16_t VALUE_COLOR = 0xCE59;   // An item's own stats, neither better nor worse
static const uint16_t BETTER_COLOR = 0x07E0;
static const uint16_t WORSE_COLOR = 0xF8A6;
static const uint16_t SAME_COLOR = 0x7BEF;

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
    : header("Inventory", "Awards", "Zones"),

      statsCard(CARD_X, CARD_Y, CARD_W, CARD_H, CARD_BORDER, false, 0x0000, true, 4),
      attackIcon(CARD_X + 8, STAT_ICON_Y, 16, 16, image_sword_02b_pixels, true, 0x0000),
      attackText(CARD_X + 28, STAT_TEXT_Y, "0", ATTACK_COLOR, 2),
      defenseIcon(CARD_X + 84, STAT_ICON_Y, 16, 16, image_armor_01b_pixels, true, 0x0000),
      defenseText(CARD_X + 104, STAT_TEXT_Y, "0", DEFENSE_COLOR, 2),
      hpIcon(CARD_X + 158, STAT_ICON_Y, 16, 16, image_upgrade_heart_pixels, true, 0x0000),
      hpText(CARD_X + 178, STAT_TEXT_Y, "0", HP_COLOR, 2),

      list(ROWS),

      detailPanel(PANEL_X, PANEL_Y, PANEL_W, PANEL_H, PANEL_BORDER, false, 0x0000, true, 4),
      panelTitle(TEXT_X, TITLE_Y, "", 0xFFFF, 1),
      panelRight(TEXT_RIGHT, TITLE_Y, "", HINT_COLOR, 1, TR_DATUM),
      actionBadge(TEXT_X, HINT_ROW_Y + BADGE_OFFSET_Y, ButtonBadge::CONFIRM),
      actionLabel(TEXT_X + ButtonBadge::SIZE + 4, HINT_ROW_Y, "", 0xFFFF, 1),
      hintText(TEXT_RIGHT, HINT_ROW_Y, "", HINT_COLOR, 1, TR_DATUM),

      game(game),
      inventory(inventory),
      sound(sound),
      mode(SLOT_LIST),
      inspecting(EquipSlot::HELMET),
      lastRefresh(0),
      lastRevision(0),
      rowsDirty(true)
{
    addElement(&header);
    setHeader(&header);

    addElement(&statsCard);
    addElement(&attackIcon);
    addElement(&attackText);
    addElement(&defenseIcon);
    addElement(&defenseText);
    addElement(&hpIcon);
    addElement(&hpText);

    for (uint8_t i = 0; i < ROWS; i++)
    {
        rows[i].setPosition(ROW_X, LIST_TOP + i * ROW_PITCH);
        rows[i].setFrameColor(ROW_FRAME);
        rows[i].setSelectedFill(SELECTED_FILL);
        addElement(&rows[i]);
    }

    addElement(&detailPanel);
    addElement(&panelTitle);
    addElement(&panelRight);
    for (uint8_t i = 0; i < STAT_COLUMNS; i++)
    {
        statColumns[i] = Text(STAT_COLUMN_X[i], STATS_Y, "", VALUE_COLOR, 1);
        addElement(&statColumns[i]);
    }
    addElement(&actionBadge);
    addElement(&actionLabel);
    addElement(&hintText);

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
    lastRefresh = 0;
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
    lastRefresh = 0;
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
                row.setTitleColor(0xFFFF);
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
            row.setTitleColor(0xFFFF);
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

void InventoryScreen::showStats(int8_t itemId, int8_t against, bool compare)
{
    static const char *const LABELS[] = {"ATK", "DEF", "HP"};

    // Values per column for both sides; the last column is whichever gold bonus the slot
    // gives (rings boost clicks, amulets production), so both sides share its kind
    int32_t mine[STAT_COLUMNS] = {0, 0, 0, 0};
    int32_t theirs[STAT_COLUMNS] = {0, 0, 0, 0};
    bool clickBonus = false;

    int8_t ids[2] = {itemId, against};
    int32_t *values[2] = {mine, theirs};
    for (uint8_t side = 0; side < 2; side++)
    {
        if (ids[side] == NO_ITEM)
        {
            continue;
        }
        uint8_t id = (uint8_t)ids[side];
        Stats s = inventory.getItemStats(id);
        uint8_t level = inventory.getItemLevel(id);
        values[side][0] = (int32_t)s.attack;
        values[side][1] = (int32_t)s.defense;
        values[side][2] = (int32_t)s.maxHp;
        if (ITEMS[id].clickBonusPercent > 0)
        {
            clickBonus = true;
            values[side][3] = (int32_t)scaleItemStat(ITEMS[id].clickBonusPercent, level);
        }
        else
        {
            values[side][3] = (int32_t)scaleItemStat(ITEMS[id].goldProductionBonusPercent, level);
        }
    }

    char buf[16];
    for (uint8_t i = 0; i < STAT_COLUMNS; i++)
    {
        Text &column = statColumns[i];
        int32_t value = compare ? mine[i] - theirs[i] : mine[i];
        bool shown = compare ? (mine[i] != 0 || theirs[i] != 0) : mine[i] != 0;
        column.setVisible(shown);
        if (!shown)
        {
            continue;
        }

        const char *label = i < 3 ? LABELS[i] : (clickBonus ? "CLK" : "GLD");
        const char *unit = i < 3 ? "" : "%";
        if (compare)
        {
            snprintf(buf, sizeof(buf), "%s %s%ld%s", label, value > 0 ? "+" : "", (long)value, unit);
            column.setColor(value > 0 ? BETTER_COLOR : value < 0 ? WORSE_COLOR : SAME_COLOR);
        }
        else
        {
            snprintf(buf, sizeof(buf), i < 3 ? "%s %ld%s" : "%s +%ld%s", label, (long)value, unit);
            column.setColor(VALUE_COLOR);
        }
        column.setText(buf);
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
        panelTitle.setColor(0xFFFF);
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
        actionLabel.setColor(found == 0 ? HINT_COLOR : 0xFFFF);
        snprintf(buf, sizeof(buf), "%u/%u items", (unsigned)inventory.countOwnedItems(), (unsigned)ITEM_COUNT);
        hintText.setText(buf);
        return;
    }

    hintText.setText("Back: slots");

    uint16_t selected = list.getSelected();
    int8_t itemId = selected == 0 ? NO_ITEM : inventory.itemForSlotAt(inspecting, (uint8_t)(selected - 1));
    if (itemId == NO_ITEM)
    {
        panelTitle.setText("Back to slots");
        panelTitle.setColor(0xFFFF);
        panelRight.setText("");
        hideStats();
        actionBadge.setDimmed(false);
        actionLabel.setText("Back");
        actionLabel.setColor(0xFFFF);
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

    actionBadge.setDimmed(isEquipped);
    actionLabel.setText(isEquipped ? "Already worn" : "Equip");
    actionLabel.setColor(isEquipped ? HINT_COLOR : 0xFFFF);
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
        lastRefresh = 0;
    }

    if (lastRefresh != 0 && now - lastRefresh < REFRESH_MS)
    {
        return;
    }
    lastRefresh = now;

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
    lastRefresh = 0;
}

void InventoryScreen::onDownPress()
{
    list.next();
    rowsDirty = true;
    lastRefresh = 0;
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
