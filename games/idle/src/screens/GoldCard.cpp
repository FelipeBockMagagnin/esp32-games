#include "GoldCard.h"
#include "../game/Format.h"
#include "../game/SatMath.h"

static constexpr Color CARD_BORDER = Theme::GOLD_FRAME;
static constexpr Color RATE_COLOR = Theme::GOOD;
static const int16_t COIN_RADIUS = 5;
static const unsigned long SPENT_BLINK_MS = 150;
static const unsigned long SPENT_SHOW_MS = 900;

GoldCard::GoldCard(int16_t x, int16_t y, int16_t w, int16_t h)
    : card(x, y, w, h, CARD_BORDER, true, Theme::CARD, true, 4),
      coin(x + 12, y + h / 2, COIN_RADIUS, COIN_RADIUS, Theme::GOLD, true),
      amount(x + 23, y + 4, "0", Theme::GOLD, 2),
      rate(x + w - 6, y + 8, "+0 gold/s", RATE_COLOR, 1, TR_DATUM),
      spent(0),
      spentAt(0),
      showingSpent(false)
{
}

void GoldCard::addTo(Screen &screen)
{
    screen.addElement(&card);
    screen.addElement(&coin, Theme::CARD);
    screen.addElement(&amount, Theme::CARD);
    screen.addElement(&rate, Theme::CARD);
}

void GoldCard::enter(const GameState &game)
{
    roll.snap(game.getGold());
    amount.setText(formatAmount(roll.get()));
    showingSpent = false;
    refresh(game);
}

void GoldCard::spend(uint64_t cost, unsigned long now)
{
    // Purchases in quick succession add up on one line
    spent = showingSpent ? satAdd(spent, cost) : cost;
    spentAt = now;
    showingSpent = true;
    char buf[24];
    snprintf(buf, sizeof(buf), "-%s", formatAmount(spent).c_str());
    rate.setText(buf);
    rate.setColor(Theme::BAD);
}

void GoldCard::animate(unsigned long now, const GameState &game)
{
    bool blink = showingSpent && now - spentAt < SPENT_BLINK_MS;
    amount.setColor(blink ? Theme::TEXT : Theme::GOLD);
    coin.setColor(blink ? Theme::TEXT : Theme::GOLD);
    if (showingSpent && now - spentAt >= SPENT_SHOW_MS)
    {
        showingSpent = false;
        refresh(game);
    }
    if (roll.step(now, game.getGold()))
    {
        amount.setText(formatAmount(roll.get()));
    }
}

void GoldCard::refresh(const GameState &game)
{
    if (showingSpent)
    {
        return;
    }
    rate.setText(formatRate(game.getProductionPerSecond()));
    rate.setColor(RATE_COLOR);
}
