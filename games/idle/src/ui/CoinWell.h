#ifndef UI_COIN_WELL_H
#define UI_COIN_WELL_H

#include "UI.h"

// The gold card's coin, where the coins flown from the ore land. The ore display cannot
// paint outside its sprite, so a coin leaves it at its top edge and finishes the flight
// here, composed in a sprite of its own so it can pass over the big coin without erasing it.
// The background is the eraseColor it is registered with.
class CoinWell : public UI
{
public:
    // (coinX, coinY) is the big coin's centre, relative to the element
    CoinWell(int16_t x, int16_t y, int16_t w, int16_t h, int16_t coinX, int16_t coinY,
             int16_t radius, int16_t pulseRadius);
    ~CoinWell();

    void pulse();
    // Coins enter at (fromX, fromY), relative to the element, and fly into the big coin
    void catchCoins(uint8_t count, int16_t fromX, int16_t fromY);
    // How many coins reached the big coin since the last call
    uint8_t takeArrivals();

    void update(unsigned long now);
    void draw(TFT_eSPI &tft) override;

private:
    static const uint8_t MAX_COINS = 8;

    struct Coin
    {
        bool active;
        unsigned long start;
        int16_t x0;
        int16_t y0;
    };

    int16_t coinX;
    int16_t coinY;
    int16_t radius;
    int16_t pulseRadius;

    Coin coins[MAX_COINS];
    uint8_t nextCoin;
    uint8_t arrivals;
    unsigned long pulseUntil;
    bool pulsing;

    unsigned long now;
    unsigned long lastFrame;
    bool animating;

    TFT_eSprite *sprite;
};

#endif // UI_COIN_WELL_H
