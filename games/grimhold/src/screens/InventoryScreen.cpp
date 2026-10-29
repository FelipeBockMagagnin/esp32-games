#include "InventoryScreen.h"
#include <string.h>
#include "../render/Art.h"

static const int16_t COLUMN_W = 154;
static const int16_t ROW_H = ListRow::DEFAULT_H;
static const int16_t WORN_X = 4;
static const int16_t BAG_X = DISPLAY_W - COLUMN_W - 4;
static const int16_t FIRST_ROW_Y = 30;
static const int16_t POTION_Y = FIRST_ROW_Y + SLOT_COUNT * ROW_H + 6;

InventoryScreen::InventoryScreen(Run &run, ScreenManager &screenManager, SoundManager &sound, Screen &pauseScreen)
    : header("Inventory"),
      hintText(WORN_X, POTION_Y + 14, "Confirm wear  Right drop", Theme::TEXT_DIM),
      potionText(WORN_X, POTION_Y, "", Theme::TEXT_2),
      run(run),
      screenManager(screenManager),
      sound(sound),
      pauseScreen(pauseScreen)
{
    addElement(&header);
    setHeader(&header);
    for (uint8_t i = 0; i < SLOT_COUNT; i++)
    {
        worn[i] = ListRow(WORN_X, FIRST_ROW_Y + i * ROW_H, COLUMN_W, ROW_H);
        addElement(&worn[i]);
    }
    for (uint8_t i = 0; i < BAG_SIZE; i++)
    {
        bag[i] = ListRow(BAG_X, FIRST_ROW_Y + i * ROW_H, COLUMN_W, ROW_H);
        addElement(&bag[i]);
    }
    addElement(&potionText);
    addElement(&hintText);
}

void InventoryScreen::fill(ListRow &row, const Item &item, const char *emptyTitle)
{
    if (item.empty())
    {
        row.setTitle(emptyTitle);
        row.setTitleColor(Theme::TEXT_DIM);
        row.setSubtitle("");
        row.clearIcon();
        row.setTrend(ListRow::TREND_NONE);
        return;
    }
    const SlotDef &def = SLOTS[item.slot];
    char stat[16], line[28];
    itemStat(item, stat, sizeof(stat));
    snprintf(line, sizeof(line), "%s %s", TIER_NAMES[item.tier], stat);
    row.setTitle(def.names[item.series % def.series]);
    row.setTitleColor(Theme::tierColor(item.tier));
    row.setSubtitle(line);
    row.setIconPixels(ITEM_ICONS[ITEM_SERIES_FIRST[item.slot] + item.series * ITEM_TIERS + item.tier], 16, 16);
}

void InventoryScreen::refresh()
{
    const Inventory &inv = run.inventory;
    const Item &chosen = inv.bag[selected];
    for (uint8_t s = 0; s < SLOT_COUNT; s++)
    {
        fill(worn[s], inv.equipped[s], SLOTS[s].name);
        worn[s].setState(!chosen.empty() && chosen.slot == s ? ListRow::OWNED : ListRow::NORMAL);
    }
    for (uint8_t i = 0; i < BAG_SIZE; i++)
    {
        const Item &item = inv.bag[i];
        fill(bag[i], item, "-");
        if (!item.empty())
        {
            uint16_t wornValue = inv.equipped[item.slot].value;
            bag[i].setTrend(item.value > wornValue   ? ListRow::TREND_UP
                            : item.value < wornValue ? ListRow::TREND_DOWN
                                                     : ListRow::TREND_NONE);
        }
        bag[i].setState(i == selected ? ListRow::SELECTED : ListRow::NORMAL);
    }
    char text[40];
    snprintf(text, sizeof(text), "Potions: %u health, %u mana", inv.hpPotions, inv.mpPotions);
    potionText.setText(text);
}

void InventoryScreen::onEnter(TFT_eSPI &tft)
{
    refresh();
    Screen::onEnter(tft);
}

void InventoryScreen::onUpPress()
{
    selected = (selected + BAG_SIZE - 1) % BAG_SIZE;
    refresh();
    sound.playMenu();
}

void InventoryScreen::onDownPress()
{
    selected = (selected + 1) % BAG_SIZE;
    refresh();
    sound.playMenu();
}

void InventoryScreen::onConfirmPress()
{
    if (run.inventory.bag[selected].empty())
    {
        sound.playError();
        return;
    }
    run.equipFromBag(selected);
    sound.playEquip();
    refresh();
}

bool InventoryScreen::onRightPress()
{
    if (run.inventory.bag[selected].empty())
    {
        sound.playError();
        return true;
    }
    run.dropFromBag(selected);
    sound.playMenu();
    refresh();
    return true;
}

bool InventoryScreen::onBackPress()
{
    sound.playMenu();
    screenManager.replaceModal(&pauseScreen);
    return true;
}
