#include "CoinWell.h"

static const unsigned long FRAME_MS = 33;
static const unsigned long COIN_MS = 130;
static const unsigned long PULSE_MS = 120;
static const int16_t COIN_RADIUS = 3; // Same as the ore display's, so the hand-off is seamless

CoinWell::CoinWell(int16_t x, int16_t y, int16_t w, int16_t h, int16_t coinX, int16_t coinY,
                   int16_t radius, int16_t pulseRadius)
    : UI(x, y, w, h),
      coinX(coinX),
      coinY(coinY),
      radius(radius),
      pulseRadius(pulseRadius),
      nextCoin(0),
      arrivals(0),
      pulseUntil(0),
      pulsing(false),
      now(0),
      lastFrame(0),
      animating(false),
      sprite(nullptr)
{
    for (uint8_t i = 0; i < MAX_COINS; i++)
    {
        coins[i].active = false;
    }
}

CoinWell::~CoinWell()
{
    if (sprite != nullptr)
    {
        sprite->deleteSprite();
        delete sprite;
    }
}

void CoinWell::pulse()
{
    pulseUntil = now + PULSE_MS;
    pulsing = true;
    markDirty();
}

// As in the ore display, a coin whose slot is reused before it lands counts as arrived
void CoinWell::catchCoins(uint8_t count, int16_t fromX, int16_t fromY)
{
    for (uint8_t n = 0; n < count; n++)
    {
        Coin &coin = coins[nextCoin];
        nextCoin = (nextCoin + 1) % MAX_COINS;
        if (coin.active)
        {
            arrivals++;
        }
        coin.active = true;
        coin.start = now;
        coin.x0 = fromX;
        coin.y0 = fromY;
    }
    lastFrame = 0;
}

uint8_t CoinWell::takeArrivals()
{
    uint8_t count = arrivals;
    arrivals = 0;
    return count;
}

void CoinWell::update(unsigned long time)
{
    now = time;

    bool landed = false;
    bool anyCoin = false;
    for (uint8_t i = 0; i < MAX_COINS; i++)
    {
        if (coins[i].active && now - coins[i].start >= COIN_MS)
        {
            coins[i].active = false;
            arrivals++;
            landed = true;
        }
        anyCoin = anyCoin || coins[i].active;
    }
    if (landed)
    {
        pulse();
    }
    if (pulsing && (long)(now - pulseUntil) >= 0)
    {
        pulsing = false;
        markDirty();
    }

    bool wasAnimating = animating;
    animating = anyCoin;
    if (animating && now - lastFrame >= FRAME_MS)
    {
        lastFrame = now;
        markDirty();
    }
    else if (wasAnimating && !animating)
    {
        markDirty();
    }
}

void CoinWell::draw(TFT_eSPI &tft)
{
    if (sprite == nullptr)
    {
        sprite = new TFT_eSprite(&tft);
        sprite->setColorDepth(16);
        if (sprite->createSprite(w, h) == nullptr)
        {
            delete sprite;
            sprite = nullptr;
        }
    }

    int16_t r = pulsing ? pulseRadius : radius;

    // No memory for the sprite: the big coin alone, without the coins flying in
    if (sprite == nullptr)
    {
        tft.fillRect(x, y, w, h, eraseColor);
        tft.fillCircle(x + coinX, y + coinY, r, Theme::GOLD);
        return;
    }

    sprite->fillSprite(eraseColor);
    sprite->fillCircle(coinX, coinY, r, Theme::GOLD);
    for (uint8_t i = 0; i < MAX_COINS; i++)
    {
        const Coin &coin = coins[i];
        if (!coin.active)
        {
            continue;
        }
        int32_t t = (int32_t)(now - coin.start) * 256 / COIN_MS;
        int16_t px = coin.x0 + (coinX - coin.x0) * t / 256;
        int16_t py = coin.y0 + (coinY - coin.y0) * t / 256;
        sprite->fillCircle(px, py, COIN_RADIUS, Theme::GOLD);
        sprite->drawCircle(px, py, COIN_RADIUS, Theme::GOLD_FRAME);
        sprite->drawPixel(px - 1, py - 1, Theme::TEXT);
    }
    sprite->pushSprite(x, y);
}
