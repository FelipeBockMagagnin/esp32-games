#include "BuildingScreen.h"
#include "../game/Format.h"

// Values change every frame while producing, so only rebuild the texts a few times per second
static const unsigned long REFRESH_MS = 100;

static const int16_t ROW_X = 5;
static const int16_t LIST_TOP = 54; // Below the gold line
static const int16_t ROW_PITCH = 28;

static const uint16_t RATE_COLOR = 0xAD55;    // Light gray, secondary to the gold amount
static const uint16_t AFFORD_COLOR = 0x07E0;  // Green price when the player can buy
static const uint16_t TOO_DEAR_COLOR = 0x7BEF; // Gray price when they cannot

BuildingScreen::BuildingScreen(GameState &game, SoundManager &sound)
    : header("Buildings", "Mining", "Upgrade"),
      goldIndicator(13, 40, 5, 5, GOLD_COLOR, true),
      goldText(21, 32, "0", 0xFFFF, 2),
      goldRate(229, 32, "+0/s", RATE_COLOR, 2, TR_DATUM),
      game(game),
      sound(sound),
      lastRefresh(0)
{
    addElement(&header);

    addElement(&goldIndicator);
    addElement(&goldText);
    addElement(&goldRate);

    for (uint8_t i = 0; i < ListView::VISIBLE_ROWS; i++)
    {
        rows[i].setPosition(ROW_X, LIST_TOP + i * ROW_PITCH);
        rows[i].setShowCoin(true);
        rows[i].setIconBitmap(image_cursor_black_white_bits, 11, 16);
        rowBuilding[i] = 0xFF;
        addElement(&rows[i]);
    }

    list.setCount(BUILDING_COUNT);
}

void BuildingScreen::update(unsigned long now)
{
    if (lastRefresh != 0 && now - lastRefresh < REFRESH_MS)
    {
        return;
    }
    lastRefresh = now;

    goldText.setText(formatAmount(game.getGold()));
    goldRate.setText(formatPerSecond(game.getProductionPerSecond()));

    uint8_t visible = list.getVisibleCount();
    for (uint8_t i = 0; i < ListView::VISIBLE_ROWS; i++)
    {
        ListRow &row = rows[i];
        if (i >= visible)
        {
            row.setVisible(false);
            continue;
        }

        uint8_t id = (uint8_t)(list.getFirstVisible() + i);
        row.setVisible(true);

        if (rowBuilding[i] != id)
        {
            rowBuilding[i] = id;
            row.setTitle(BUILDINGS[id].name);
            row.setSubtitle(formatRate(BUILDINGS[id].productionPerLevel));
        }

        char buf[16];
        row.setValue(formatAmount(game.getBuildingCost(id)));
        snprintf(buf, sizeof(buf), "x%u", (unsigned)game.getBuildingLevel(id));
        row.setValueSub(buf);
        row.setValueColor(game.canAffordBuilding(id) ? AFFORD_COLOR : TOO_DEAR_COLOR);
        row.setState(id == list.getSelected() ? ListRow::SELECTED : ListRow::NORMAL);
    }
}

void BuildingScreen::onSelectPress()
{
    list.next();
    lastRefresh = 0;
}

void BuildingScreen::onConfirmPress()
{
    // buyBuilding only spends gold when the player can afford it
    if (game.buyBuilding((uint8_t)list.getSelected()))
    {
        sound.playBuy();
    }
    else
    {
        sound.playError();
    }
    lastRefresh = 0;
}
