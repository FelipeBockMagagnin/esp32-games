#include "ShopScreen.h"
#include "../game/Format.h"

static const int16_t COINS_Y = 34;
static const int16_t FIRST_ROW_Y = 48;
static const int16_t ROW_PITCH = ListRow::DEFAULT_H + 3;

ShopScreen::ShopScreen(World &world, Colony &colony, SoundManager &sound)
    : header("Shop"),
      coinsText(235, COINS_Y, "", Theme::GOLD, 1, TR_DATUM),
      list(ROW_COUNT),
      world(world),
      colony(colony),
      sound(sound)
{
    addElement(&header);
    setHeader(&header);
    addElement(&coinsText);
    for (uint8_t i = 0; i < ROW_COUNT; i++)
    {
        rows[i].setPosition(5, FIRST_ROW_Y + i * ROW_PITCH);
        addElement(&rows[i]);
    }
    list.setCount(ENTRY_COUNT);
}

void ShopScreen::onEnter(TFT_eSPI &tft)
{
    refresh();
    Screen::onEnter(tft);
}

void ShopScreen::update(unsigned long now)
{
    if (!refreshDue(now))
    {
        return;
    }
    refresh();
}

void ShopScreen::refresh()
{
    char buf[24];
    snprintf(buf, sizeof(buf), "%s coins", formatAmount(world.getCoins()).c_str());
    coinsText.setText(buf);
    for (uint8_t r = 0; r < ROW_COUNT; r++)
    {
        uint16_t entry = list.getFirstVisible() + r;
        rows[r].setVisible(r < list.getVisibleCount());
        if (r < list.getVisibleCount())
        {
            fillRow(rows[r], entry, entry == list.getSelected());
        }
    }
}

void ShopScreen::fillRow(ListRow &row, uint8_t entry, bool selected)
{
    char buf[32];
    uint32_t price = 0;
    uint8_t needs = 0;
    bool owned = false;
    bool full = false;
    if (entry < SPECIES_COUNT)
    {
        const SpeciesDef &def = SPECIES[entry];
        snprintf(buf, sizeof(buf), "Adopt %s", def.name);
        row.setTitle(buf);
        price = colony.adoptPrice(entry);
        needs = def.reqPopulation;
        full = colony.getCount() >= colony.getCapacity();
        snprintf(buf, sizeof(buf), "%u/min, eats %u", (unsigned)def.coinsPerMinute, (unsigned)def.mealFood);
    }
    else if (entry < SPECIES_COUNT + SEED_COUNT)
    {
        uint8_t c = entry - SPECIES_COUNT + 1;
        const CropDef &crop = CROPS[c];
        snprintf(buf, sizeof(buf), "%s seeds", crop.name);
        row.setTitle(buf);
        price = crop.unlockPrice;
        owned = world.isCropUnlocked(c);
        snprintf(buf, sizeof(buf), "%us, +%u food", (unsigned)crop.growSeconds, (unsigned)crop.food);
    }
    else
    {
        uint8_t u = entry - SPECIES_COUNT - SEED_COUNT;
        const UpgradeDef &def = UPGRADES[u];
        row.setTitle(def.name);
        price = def.price;
        needs = def.reqPopulation;
        owned = world.hasUpgrade(u);
        snprintf(buf, sizeof(buf), "%s", def.note);
    }

    bool locked = colony.getCount() < needs;
    if (locked)
    {
        snprintf(buf, sizeof(buf), "Needs %u critters", (unsigned)needs);
    }
    else if (full && !owned)
    {
        snprintf(buf, sizeof(buf), "Needs a free bed");
    }
    row.setSubtitle(buf);
    bool affordable = world.getCoins() >= price;
    row.setValue(owned ? String("owned") : formatAmount(price));
    row.setValueColor(owned ? Theme::GOOD : (affordable ? Theme::GOLD : Theme::BAD));
    row.setShowCoin(!owned);
    if (selected)
    {
        row.setState(ListRow::SELECTED);
    }
    else if (owned)
    {
        row.setState(ListRow::OWNED);
    }
    else
    {
        row.setState(locked || full || !affordable ? ListRow::DIMMED : ListRow::NORMAL);
    }
}

void ShopScreen::onUpPress()
{
    list.previous();
    sound.playMenu();
    forceRefresh();
}

void ShopScreen::onDownPress()
{
    list.next();
    sound.playMenu();
    forceRefresh();
}

void ShopScreen::onConfirmPress()
{
    uint8_t entry = list.getSelected();
    bool done;
    if (entry < SPECIES_COUNT)
    {
        done = colony.adopt(entry);
    }
    else if (entry < SPECIES_COUNT + SEED_COUNT)
    {
        done = world.unlockCrop(entry - SPECIES_COUNT + 1);
    }
    else
    {
        done = world.buyUpgrade(entry - SPECIES_COUNT - SEED_COUNT, colony.getCount());
    }
    if (done)
    {
        sound.playBuy();
    }
    else
    {
        sound.playError();
    }
    forceRefresh();
}
