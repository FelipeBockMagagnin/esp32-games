#include "InventoryScreen.h"
#include "../game/Stats.h"

static const unsigned long REFRESH_MS = 100;

static const int16_t ROW_X = 5;
static const int16_t LIST_TOP = 32;
static const int16_t ROW_PITCH = 28;

// Totals row along the bottom, below the list
static const int16_t FOOTER_ICON_Y = 296;
static const int16_t FOOTER_TEXT_Y = 297;

// Compact "atk 12 def 4 hp 30", skipping whatever the item does not give
static String describeStats(const Stats &s)
{
    char buf[40];
    size_t n = 0;

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
        snprintf(buf + n, sizeof(buf) - n, "%shp %u", n > 0 ? " " : "", (unsigned)s.maxHp);
    }
    else if (n == 0)
    {
        buf[0] = '\0';
    }

    return String(buf);
}

InventoryScreen::InventoryScreen(GameState &game, Inventory &inventory, SoundManager &sound)
    : header("Inventory", "Awards", "Zones"),

      attackIcon(8, FOOTER_ICON_Y, 16, 16, image_sword_02b_pixels),
      attackText(28, FOOTER_TEXT_Y, "0", 0xFFFF, 2),
      defenseIcon(88, FOOTER_ICON_Y, 16, 16, image_armor_01b_pixels),
      defenseText(108, FOOTER_TEXT_Y, "0", 0xFFFF, 2),
      hpIcon(166, FOOTER_ICON_Y, 15, 16, image_cards_hearts_bits),
      hpText(185, FOOTER_TEXT_Y, "0", 0xFFFF, 2),

      game(game),
      inventory(inventory),
      sound(sound),
      mode(SLOT_LIST),
      inspecting(EquipSlot::HELMET),
      lastRefresh(0),
      lastRevision(0)
{
    addElement(&header);
    setHeader(&header);

    for (uint8_t i = 0; i < ListView::VISIBLE_ROWS; i++)
    {
        rows[i].setPosition(ROW_X, LIST_TOP + i * ROW_PITCH);
        addElement(&rows[i]);
    }

    addElement(&attackIcon);
    addElement(&attackText);
    addElement(&defenseIcon);
    addElement(&defenseText);
    addElement(&hpIcon);
    addElement(&hpText);

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
    lastRefresh = 0;
}

void InventoryScreen::refreshRows()
{
    uint8_t visible = list.getVisibleCount();

    for (uint8_t i = 0; i < ListView::VISIBLE_ROWS; i++)
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
                row.setTitle(EQUIP_SLOT_NAMES[index]);
                row.setSubtitle("empty");
                row.setValue("");
                row.clearIcon();
                row.setState(isSelected ? ListRow::SELECTED : ListRow::DIMMED);
            }
            else
            {
                const ItemDef &def = ITEMS[itemId];
                row.setTitle(def.name);
                row.setSubtitle(describeStats(inventory.getItemStats((uint8_t)itemId)));
                char levelBuf[12];
                snprintf(levelBuf, sizeof(levelBuf), "Lv %u", (unsigned)inventory.getItemLevel((uint8_t)itemId));
                row.setValue(levelBuf);
                if (def.iconPixels != nullptr)
                {
                    row.setIconPixels(def.iconPixels, def.iconW, def.iconH);
                }
                else
                {
                    row.setIconBitmap(def.iconBitmap, def.iconW, def.iconH);
                }
                row.setState(isSelected ? ListRow::SELECTED : ListRow::NORMAL);
            }
            row.setValueSub("");
            continue;
        }

        if (index == 0)
        {
            row.setTitle("< Back");
            row.setSubtitle("");
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

        row.setTitle(def.name);
        row.setSubtitle(describeStats(inventory.getItemStats((uint8_t)itemId)));
        char levelBuf[12];
                snprintf(levelBuf, sizeof(levelBuf), "Lv %u", (unsigned)inventory.getItemLevel((uint8_t)itemId));
                row.setValue(levelBuf);
        row.setValueSub(isEquipped ? "worn" : "");
        if (def.iconPixels != nullptr)
        {
            row.setIconPixels(def.iconPixels, def.iconW, def.iconH);
        }
        else
        {
            row.setIconBitmap(def.iconBitmap, def.iconW, def.iconH);
        }

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
        lastRefresh = 0;
    }

    if (lastRefresh != 0 && now - lastRefresh < REFRESH_MS)
    {
        return;
    }
    lastRefresh = now;

    refreshRows();
    refreshTotals();
}

void InventoryScreen::onSelectPress()
{
    list.next();
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
