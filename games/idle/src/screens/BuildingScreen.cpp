#include "BuildingScreen.h"
#include "../game/Format.h"

// Values change every frame while producing, so only rebuild the texts a few times per second

static const uint16_t CARD_BORDER = Theme::GOLD_FRAME;    // Dark amber, same frame as the Mining gold card
static const uint16_t RATE_COLOR = Theme::GOOD;     // Soft green: income that grows on its own
static const uint16_t AFFORD_COLOR = Theme::GOOD;   // Green price when the player can buy
static const uint16_t TOO_DEAR_COLOR = Theme::TEXT_DIM; // Gray price when they cannot
static const uint16_t PANEL_BORDER = Theme::FRAME;
static const uint16_t HINT_COLOR = Theme::TEXT_2;
static const uint16_t SHARE_COLOR = Theme::GOLD_BAR;    // Amber fill, dark enough for the white label
static const uint16_t SHARE_BG = Theme::GOLD_TRACK;
static const uint16_t SHARE_BORDER = Theme::GOLD_BORDER;

// Gold card
static const int16_t CARD_X = 5;
static const int16_t CARD_Y = 31;
static const int16_t CARD_W = 230;
static const int16_t CARD_H = 22;
static const int16_t COIN_RADIUS = 5;

// List
static const int16_t ROW_X = 5;
static const int16_t LIST_TOP = 57;
static const int16_t ROW_PITCH = 28;

// Detail panel under the list; text (8px) and badge (11px) are centred on the row's middle
static const int16_t PANEL_X = 5;
static const int16_t PANEL_Y = 283;
static const int16_t PANEL_W = 230;
static const int16_t PANEL_H = 35;
static const int16_t BUY_ROW_Y = PANEL_Y + 5;
static const int16_t BADGE_OFFSET_Y = (8 - ButtonBadge::SIZE) / 2;
static const int16_t SHARE_Y = PANEL_Y + 17;
static const int16_t SHARE_H = 13;

BuildingScreen::BuildingScreen(GameState &game, SoundManager &sound)
    : header("Buildings"),

      goldCard(CARD_X, CARD_Y, CARD_W, CARD_H, CARD_BORDER, true, Theme::CARD, true, 4),
      goldCoin(CARD_X + 12, CARD_Y + CARD_H / 2, COIN_RADIUS, COIN_RADIUS, Theme::GOLD, true),
      goldText(CARD_X + 23, CARD_Y + 4, "0", Theme::GOLD, 2),
      goldRate(CARD_X + CARD_W - 6, CARD_Y + 8, "+0 gold/s", RATE_COLOR, 1, TR_DATUM),

      list(ROWS),

      detailPanel(PANEL_X, PANEL_Y, PANEL_W, PANEL_H, PANEL_BORDER, true, Theme::CARD, true, 3),
      buyBadge(PANEL_X + 5, BUY_ROW_Y + BADGE_OFFSET_Y, ButtonBadge::CONFIRM),
      buyLabel(PANEL_X + 20, BUY_ROW_Y, "Buy", Theme::TEXT, 1),
      buyDetail(PANEL_X + PANEL_W - 5, BUY_ROW_Y, "", HINT_COLOR, 1, TR_DATUM),
      shareBar(PANEL_X + 5, SHARE_Y, PANEL_W - 10, SHARE_H, 0, 100, SHARE_COLOR, SHARE_BG, SHARE_BORDER),

      game(game),
      sound(sound)
{
    addElement(&header);
    setHeader(&header);

    addElement(&goldCard);
    addElement(&goldCoin, Theme::CARD);
    addElement(&goldText, Theme::CARD);
    addElement(&goldRate, Theme::CARD);

    for (uint8_t i = 0; i < ROWS; i++)
    {
        rows[i].setPosition(ROW_X, LIST_TOP + i * ROW_PITCH);
        rows[i].setShowCoin(true);
        rowBuilding[i] = 0xFF;
        rowRevealed[i] = false;
        addElement(&rows[i]);
    }

    addElement(&detailPanel);
    addElement(&buyBadge, Theme::CARD);
    addElement(&buyLabel, Theme::CARD);
    addElement(&buyDetail, Theme::CARD);
    addElement(&shareBar, Theme::CARD);

    list.setCount(BUILDING_COUNT);
}

// Cookie Clicker style: a building shows itself once the one before it is owned, so the
// list unfolds as the player progresses. Hidden ones keep their price, and can still be
// bought, so the player can see how far off the next one is.
bool BuildingScreen::isRevealed(uint8_t id) const
{
    return id == 0 || game.getBuildingLevel(id) > 0 || game.getBuildingLevel(id - 1) > 0;
}

void BuildingScreen::update(unsigned long now)
{
    if (!refreshDue(now))
    {
        return;
    }

    goldText.setText(formatAmount(game.getGold()));
    goldRate.setText(formatRate(game.getProductionPerSecond()));

    uint8_t visible = list.getVisibleCount();
    for (uint8_t i = 0; i < ROWS; i++)
    {
        ListRow &row = rows[i];
        if (i >= visible)
        {
            row.setVisible(false);
            continue;
        }

        uint8_t id = (uint8_t)(list.getFirstVisible() + i);
        bool revealed = isRevealed(id);
        row.setVisible(true);

        if (rowBuilding[i] != id || rowRevealed[i] != revealed)
        {
            rowBuilding[i] = id;
            rowRevealed[i] = revealed;
            if (revealed)
            {
                row.setTitle(BUILDINGS[id].name);
                row.setIconPixels(BUILDINGS[id].icon, 16, 16);
            }
            else
            {
                row.setTitle("???");
                row.setSubtitle("Not discovered yet");
                row.setValueSub("");
                row.clearIcon();
            }
        }

        char buf[24];
        if (revealed)
        {
            // Rate per level includes the production bonuses, so it is what buying one adds
            snprintf(buf, sizeof(buf), "%s each", formatPerSecond(game.getBuildingProductionPerLevel(id)).c_str());
            row.setSubtitle(buf);
            snprintf(buf, sizeof(buf), "x%u", (unsigned)game.getBuildingLevel(id));
            row.setValueSub(buf);
        }

        row.setValue(formatAmount(game.getBuildingCost(id)));
        row.setValueColor(game.canAffordBuilding(id) ? AFFORD_COLOR : TOO_DEAR_COLOR);

        ListRow::State state = revealed ? ListRow::NORMAL : ListRow::DIMMED;
        row.setState(id == list.getSelected() ? ListRow::SELECTED : state);
    }

    updateDetail();
}

void BuildingScreen::updateDetail()
{
    uint8_t id = (uint8_t)list.getSelected();
    char buf[40];

    if (game.canAffordBuilding(id))
    {
        snprintf(buf, sizeof(buf), "%s", formatRate(game.getBuildingProductionPerLevel(id)).c_str());
        buyDetail.setColor(AFFORD_COLOR);
    }
    else
    {
        // Not affordable means the cost is above the balance, so this cannot underflow
        uint64_t missing = game.getBuildingCost(id) - game.getGold();
        snprintf(buf, sizeof(buf), "need %s more", formatAmount(missing).c_str());
        buyDetail.setColor(HINT_COLOR);
    }
    buyDetail.setText(buf);
    buyBadge.setDimmed(!game.canAffordBuilding(id));

    uint64_t produced = game.getBuildingProduction(id);
    uint64_t total = game.getProductionPerSecond();
    uint32_t share = total > 0 ? (uint32_t)(produced * 100 / total) : 0;
    if (share > 100)
    {
        share = 100; // Per-building rounding can land a hair above the total
    }
    shareBar.setProgress(share, 100);

    if (produced == 0)
    {
        shareBar.setLabel("None built yet");
    }
    else
    {
        snprintf(buf, sizeof(buf), "%s  (%u%% of income)", formatPerSecond(produced).c_str(), (unsigned)share);
        shareBar.setLabel(buf);
    }
}

void BuildingScreen::onUpPress()
{
    list.previous();
    forceRefresh();
}

void BuildingScreen::onDownPress()
{
    list.next();
    forceRefresh();
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
    forceRefresh();
}
