#ifndef GOLD_CARD_H
#define GOLD_CARD_H

#include "Screen.h"
#include "Box.h"
#include "Ellipse.h"
#include "Text.h"
#include "../ui/GoldRoll.h"
#include "../game/GameState.h"

// The shops' strip of balance and production. A purchase rolls the balance down to what is
// left and shows what it cost in the rate's place for a moment.
class GoldCard
{
private:
    Box card;
    Ellipse coin;
    Text amount;
    Text rate;

    GoldRoll roll;
    uint64_t spent;
    unsigned long spentAt;
    bool showingSpent;

public:
    GoldCard(int16_t x, int16_t y, int16_t w, int16_t h);

    void addTo(Screen &screen);
    void enter(const GameState &game);
    void spend(uint64_t cost, unsigned long now);
    void animate(unsigned long now, const GameState &game);
    void refresh(const GameState &game); // The rate, at the screen's text refresh
};

#endif // GOLD_CARD_H
