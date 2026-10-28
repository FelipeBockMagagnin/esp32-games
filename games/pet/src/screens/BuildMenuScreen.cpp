#include "BuildMenuScreen.h"
#include "../game/Format.h"

static const int16_t ROW_X = 5;
static const int16_t FIRST_ROW_Y = 40;
static const int16_t ROW_PITCH = ListRow::DEFAULT_H + 6;
static const int16_t HINT_Y = 300;

BuildMenuScreen::BuildMenuScreen(World &world, ScreenManager &screenManager, SoundManager &sound)
    : header(""),
      hintText(120, HINT_Y, "Confirm do it - Left back", Theme::TEXT_DIM, 1, TC_DATUM),
      world(world),
      screenManager(screenManager),
      sound(sound),
      tile(0),
      optionCount(0),
      selected(0)
{
    addElement(&header);
    setHeader(&header);
    for (uint8_t i = 0; i < MAX_OPTIONS; i++)
    {
        rows[i].setPosition(ROW_X, FIRST_ROW_Y + i * ROW_PITCH);
        addElement(&rows[i]);
    }
    addElement(&hintText);
}

void BuildMenuScreen::openFor(uint16_t t)
{
    tile = t;
    screenManager.openModal(this);
}

void BuildMenuScreen::buildOptions()
{
    optionCount = 0;
    switch (world.kindAt(tile))
    {
    case TileKind::GRASS:
        header.setTitle("Build");
        for (uint8_t b = 0; b < BUILD_COUNT; b++)
        {
            options[optionCount++] = {Action::BUILD, b};
        }
        return;
    case TileKind::SOIL:
        header.setTitle("Plant");
        for (uint8_t c = 0; c < CROP_COUNT; c++)
        {
            if (world.isCropUnlocked(c))
            {
                options[optionCount++] = {Action::PLANT, c};
            }
        }
        break;
    default:
        header.setTitle(world.kindAt(tile) == TileKind::CROP ? "Crop" : "Tile");
        break;
    }
    options[optionCount++] = {Action::DEMOLISH, 0};
}

uint32_t BuildMenuScreen::priceOf(const Option &option) const
{
    switch (option.action)
    {
    case Action::BUILD:
        return world.buildPrice((BuildId)option.index);
    case Action::PLANT:
        return CROPS[option.index].seedPrice;
    default:
        return 0;
    }
}

void BuildMenuScreen::refreshRows()
{
    char buf[32];
    for (uint8_t i = 0; i < MAX_OPTIONS; i++)
    {
        ListRow &row = rows[i];
        row.setVisible(i < optionCount);
        if (i >= optionCount)
        {
            continue;
        }
        const Option &option = options[i];
        uint32_t price = priceOf(option);
        switch (option.action)
        {
        case Action::BUILD:
            row.setTitle(BUILDS[option.index].name);
            row.setSubtitle(BUILDS[option.index].note);
            break;
        case Action::PLANT:
        {
            const CropDef &crop = CROPS[option.index];
            snprintf(buf, sizeof(buf), "Plant %s", crop.name);
            row.setTitle(buf);
            snprintf(buf, sizeof(buf), "%us, +%u food, regrows", (unsigned)crop.growSeconds,
                     (unsigned)world.harvestFood(option.index));
            row.setSubtitle(buf);
            break;
        }
        case Action::DEMOLISH:
            row.setTitle(world.kindAt(tile) == TileKind::CROP ? "Dig up" : "Clear to grass");
            row.setSubtitle(world.kindAt(tile) == TileKind::CROP ? "The crop is lost" : "Nothing is refunded");
            break;
        }
        bool affordable = world.getCoins() >= price;
        row.setValue(price == 0 ? String("free") : formatAmount(price));
        row.setValueColor(affordable ? Theme::GOLD : Theme::BAD);
        row.setShowCoin(price > 0);
        row.setState(i == selected ? ListRow::SELECTED : (affordable ? ListRow::NORMAL : ListRow::DIMMED));
    }
}

void BuildMenuScreen::onEnter(TFT_eSPI &tft)
{
    selected = 0;
    buildOptions();
    refreshRows();
    Screen::onEnter(tft);
}

void BuildMenuScreen::update(unsigned long now)
{
    if (!refreshDue(now))
    {
        return;
    }
    refreshRows();
}

void BuildMenuScreen::onUpPress()
{
    selected = selected > 0 ? selected - 1 : optionCount - 1;
    sound.playMenu();
    forceRefresh();
}

void BuildMenuScreen::onDownPress()
{
    selected = (selected + 1) % optionCount;
    sound.playMenu();
    forceRefresh();
}

void BuildMenuScreen::onConfirmPress()
{
    const Option &option = options[selected];
    bool done = false;
    switch (option.action)
    {
    case Action::BUILD:
        done = world.build(tile, (BuildId)option.index);
        break;
    case Action::PLANT:
        done = world.plant(tile, option.index);
        break;
    case Action::DEMOLISH:
        done = world.demolish(tile);
        break;
    }
    if (!done)
    {
        sound.playError();
        return;
    }
    sound.playBuy();
    screenManager.closeModal();
}

bool BuildMenuScreen::onBackPress()
{
    sound.playMenu();
    screenManager.closeModal();
    return true;
}
