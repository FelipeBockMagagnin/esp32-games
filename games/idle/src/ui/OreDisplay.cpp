#include "OreDisplay.h"
#include "ButtonBadge.h"

// ~30 fps is smooth enough and leaves the SPI bus free for the rest of the screen
static const unsigned long FRAME_MS = 33;

static const unsigned long SHAKE_STEP_MS = 20;
static const int8_t SHAKE_OFFSETS[] = {-4, 4, -3, 3, -2, 2, -1, 1};
static const uint8_t SHAKE_STEPS = sizeof(SHAKE_OFFSETS) / sizeof(SHAKE_OFFSETS[0]);

static const unsigned long POPUP_MS = 700;
static const int16_t POPUP_RISE = 45;          // Pixels travelled over its lifetime
static const unsigned long POPUP_FADE_MS = 300; // Fades out during the last part
static const int16_t POPUP_SPREAD = 20;         // Random horizontal jitter so fast clicks don't stack

static const unsigned long CHIP_MS = 450;
static const int16_t CHIP_GRAVITY = 900; // Pixels per second squared
static const int16_t CHIP_SPREAD_X = 24;
static const int16_t CHIP_SPEED_X = 140;
static const int16_t CHIP_SPEED_UP_MIN = 120;
static const int16_t CHIP_SPEED_UP_MAX = 300;
static const int16_t CHIP_SIZE = 3;

static const unsigned long COIN_MS = 480;
static const unsigned long COIN_STAGGER_MS = 55;
static const int16_t COIN_SPREAD = 20;
static const int16_t COIN_ARC = 50; // How far above the straight line the control point sits
static const int16_t COIN_RADIUS = 3;

// Halo behind the ore: rings from the outside in, each a stronger blend of the glow
// color into the background. Radii are relative to the ore's size.
static const uint8_t GLOW_RINGS = 3;
static const uint8_t GLOW_RADIUS_PERCENT[GLOW_RINGS] = {56, 46, 36};
static const uint8_t GLOW_ALPHA[GLOW_RINGS] = {28, 48, 72};

// Flat shadow under the ore, so it reads as resting on the ground rather than floating
static const uint8_t SHADOW_Y_PERCENT = 86;   // Down from the ore's top
static const uint8_t SHADOW_RX_PERCENT = 40;
static const int16_t SHADOW_RY = 5;
static const uint8_t SHADOW_ALPHA = 200; // Background blended over the halo

// The vein beside the ore: its icon at 2x over a glow that pulses, and the Up button under
// it. It only needs a slow frame rate, which keeps a waiting vein cheap on the SPI bus.
static const unsigned long VEIN_FRAME_MS = 100;
static const uint8_t VEIN_SCALE = 2;
static const int16_t VEIN_MARGIN = 4;
static const int16_t VEIN_TOP = 14;
static const unsigned long VEIN_PULSE_MS = 1200;
static const unsigned long VEIN_BLINK_MS = 200;

static uint16_t swapBytes(uint16_t color)
{
    return (uint16_t)((color >> 8) | (color << 8));
}

OreDisplay::OreDisplay(int16_t x, int16_t y, int16_t w, int16_t h,
                       const uint16_t *orePixels, int16_t oreX, int16_t oreY, int16_t oreW, int16_t oreH,
                       uint16_t popupColor)
    : UI(x, y, w, h),
      orePixels(orePixels),
      oreX(oreX),
      oreY(oreY),
      oreW(oreW),
      oreH(oreH),
      popupColor(popupColor),
      paletteSource(nullptr),
      paletteTarget(nullptr),
      paletteCount(0),
      glowColor(0x0000),
      nextPopup(0),
      nextChip(0),
      nextCoin(0),
      arrivals(0),
      shaking(false),
      shakeStart(0),
      veinIcon(nullptr),
      veinLeft(false),
      veinFading(false),
      lastVeinFrame(0),
      now(0),
      lastFrame(0),
      animating(false),
      sprite(nullptr)
{
    for (uint8_t i = 0; i < MAX_POPUPS; i++)
    {
        popups[i].active = false;
    }
    for (uint8_t i = 0; i < MAX_CHIPS; i++)
    {
        chips[i].active = false;
    }
    for (uint8_t i = 0; i < MAX_COINS; i++)
    {
        coins[i].active = false;
    }
}

OreDisplay::~OreDisplay()
{
    if (sprite != nullptr)
    {
        sprite->deleteSprite();
        delete sprite;
    }
}

void OreDisplay::shake()
{
    shaking = true;
    shakeStart = now;
    lastFrame = 0; // Show the first frame right away
}

void OreDisplay::addPopup(const String &text)
{
    addPopup(text, popupColor);
}

void OreDisplay::addPopup(const String &text, uint16_t color)
{
    // Reuses the oldest slot when all are in use
    Popup &popup = popups[nextPopup];
    nextPopup = (nextPopup + 1) % MAX_POPUPS;

    popup.active = true;
    popup.start = now;
    popup.x = oreX + oreW / 2 + random(-POPUP_SPREAD, POPUP_SPREAD + 1);
    popup.color = color;
    popup.text = text;
    lastFrame = 0;
}

void OreDisplay::burst(uint8_t count, bool gold)
{
    for (uint8_t n = 0; n < count; n++)
    {
        Chip &chip = chips[nextChip];
        nextChip = (nextChip + 1) % MAX_CHIPS;

        chip.active = true;
        chip.start = now;
        chip.x = oreX + oreW / 2 + random(-CHIP_SPREAD_X, CHIP_SPREAD_X + 1);
        chip.y = oreY + oreH / 3 + random(0, oreH / 4);
        chip.vx = random(-CHIP_SPEED_X, CHIP_SPEED_X + 1);
        chip.vy = -random(CHIP_SPEED_UP_MIN, CHIP_SPEED_UP_MAX + 1);
        if (gold)
        {
            chip.color = n % 2 == 0 ? Theme::GOLD : Theme::TEXT;
        }
        else if (paletteTarget != nullptr)
        {
            chip.color = paletteTarget[random(0, paletteCount)];
        }
        else
        {
            chip.color = popupColor;
        }
    }
    lastFrame = 0;
}

static int16_t clampCoin(int16_t value, int16_t size)
{
    return value < COIN_RADIUS ? COIN_RADIUS : value > size - 1 - COIN_RADIUS ? size - 1 - COIN_RADIUS : value;
}

// A coin whose slot is reused before it lands still counts as arrived, so a caller that
// waits for its coins never waits forever. Coins are painted in the sprite, so the target and
// the arc are kept inside it (a quadratic arc never leaves the hull of its three points).
void OreDisplay::flyCoins(uint8_t count, int16_t targetX, int16_t targetY)
{
    targetX = clampCoin(targetX, w);
    targetY = clampCoin(targetY, h);
    for (uint8_t n = 0; n < count; n++)
    {
        Coin &coin = coins[nextCoin];
        nextCoin = (nextCoin + 1) % MAX_COINS;
        if (coin.active)
        {
            arrivals++;
        }

        coin.active = true;
        coin.start = now + n * COIN_STAGGER_MS;
        coin.x0 = oreX + oreW / 2 + random(-COIN_SPREAD, COIN_SPREAD + 1);
        coin.y0 = oreY + oreH / 2 + random(-COIN_SPREAD / 2, COIN_SPREAD / 2 + 1);
        coin.x1 = targetX;
        coin.y1 = targetY;
        coin.cx = clampCoin((coin.x0 + targetX) / 2 + random(-COIN_SPREAD, COIN_SPREAD + 1), w);
        coin.cy = clampCoin((coin.y0 < targetY ? coin.y0 : targetY) - COIN_ARC + random(0, COIN_SPREAD + 1), h);
    }
    lastFrame = 0;
}

uint8_t OreDisplay::takeArrivals()
{
    uint8_t count = arrivals;
    arrivals = 0;
    return count;
}

void OreDisplay::setPalette(const uint16_t *source, const uint16_t *target, uint8_t count)
{
    if (paletteSource != source || paletteTarget != target || paletteCount != count)
    {
        paletteSource = source;
        paletteTarget = target;
        paletteCount = count;
        markDirty();
    }
}

void OreDisplay::setGlowColor(uint16_t color)
{
    if (glowColor != color)
    {
        glowColor = color;
        markDirty();
    }
}

void OreDisplay::setVein(const uint16_t *icon, bool leftSide, bool fading)
{
    if (veinIcon != icon || veinLeft != leftSide || veinFading != fading)
    {
        veinIcon = icon;
        veinLeft = leftSide;
        veinFading = fading;
        markDirty();
    }
}

void OreDisplay::clearVein()
{
    if (veinIcon != nullptr)
    {
        veinIcon = nullptr;
        markDirty();
    }
}

void OreDisplay::update(unsigned long time)
{
    now = time;

    // A waiting vein pulses on its own slow clock, apart from the click animations
    if (veinIcon != nullptr && now - lastVeinFrame >= VEIN_FRAME_MS)
    {
        lastVeinFrame = now;
        markDirty();
    }

    if (shaking && now - shakeStart >= SHAKE_STEP_MS * SHAKE_STEPS)
    {
        shaking = false;
    }

    bool anyEffect = false;
    for (uint8_t i = 0; i < MAX_POPUPS; i++)
    {
        if (popups[i].active && now - popups[i].start >= POPUP_MS)
        {
            popups[i].active = false;
        }
        anyEffect = anyEffect || popups[i].active;
    }

    for (uint8_t i = 0; i < MAX_CHIPS; i++)
    {
        if (chips[i].active && now - chips[i].start >= CHIP_MS)
        {
            chips[i].active = false;
        }
        anyEffect = anyEffect || chips[i].active;
    }

    for (uint8_t i = 0; i < MAX_COINS; i++)
    {
        if (coins[i].active && (long)(now - coins[i].start) >= (long)COIN_MS)
        {
            coins[i].active = false;
            arrivals++;
        }
        anyEffect = anyEffect || coins[i].active;
    }

    bool wasAnimating = animating;
    animating = shaking || anyEffect;

    if (animating && now - lastFrame >= FRAME_MS)
    {
        lastFrame = now;
        markDirty();
    }
    else if (wasAnimating && !animating)
    {
        // Final frame, drawn right away, clears the last popup and recenters the ore
        markDirty();
    }
}

int16_t OreDisplay::shakeOffset() const
{
    if (!shaking)
    {
        return 0;
    }
    uint8_t step = (now - shakeStart) / SHAKE_STEP_MS;
    return step < SHAKE_STEPS ? SHAKE_OFFSETS[step] : 0;
}

void OreDisplay::drawBackdrop(TFT_eSPI &tft)
{
    int16_t cx = oreX + oreW / 2;
    int16_t cy = oreY + oreH / 2;
    int16_t size = oreW < oreH ? oreW : oreH;

    for (uint8_t i = 0; i < GLOW_RINGS; i++)
    {
        uint16_t ring = tft.alphaBlend(GLOW_ALPHA[i], glowColor, eraseColor);
        sprite->fillCircle(cx, cy, size * GLOW_RADIUS_PERCENT[i] / 100, ring);
    }

    uint16_t shadow = tft.alphaBlend(SHADOW_ALPHA, eraseColor, tft.alphaBlend(GLOW_ALPHA[GLOW_RINGS - 1], glowColor, eraseColor));
    sprite->fillEllipse(cx, oreY + oreH * SHADOW_Y_PERCENT / 100, size * SHADOW_RX_PERCENT / 100, SHADOW_RY, shadow);
}

// Copies the ore into the sprite pixel by pixel: the sprite's own pushImage has no
// transparent variant and cannot recolor. The sprite buffer and the image array both
// hold byte-swapped words (pushImage copies one into the other untouched), so pixels
// are swapped to compare against the palette and swapped back to store.
void OreDisplay::drawOre(int16_t left)
{
    uint16_t *buffer = (uint16_t *)sprite->getPointer();
    for (int16_t row = 0; row < oreH; row++)
    {
        int16_t sy = oreY + row;
        if (sy < 0 || sy >= h)
        {
            continue;
        }
        for (int16_t col = 0; col < oreW; col++)
        {
            int16_t sx = left + col;
            uint16_t raw = pgm_read_word(orePixels + row * oreW + col);
            if (raw == 0 || sx < 0 || sx >= w)
            {
                continue;
            }

            if (paletteTarget != nullptr)
            {
                uint16_t color = swapBytes(raw);
                for (uint8_t i = 0; i < paletteCount; i++)
                {
                    if (paletteSource[i] == color)
                    {
                        raw = swapBytes(paletteTarget[i]);
                        break;
                    }
                }
            }
            buffer[sy * w + sx] = raw;
        }
    }
}

// Drawn into the sprite like everything else, so it can sit in the ore's area without the
// two erasing each other. The glow breathes, and the whole vein blinks when about to go.
void OreDisplay::drawVein(TFT_eSPI &tft)
{
    if (veinIcon == nullptr || (veinFading && (now / VEIN_BLINK_MS) % 2 == 1))
    {
        return;
    }

    int16_t size = 16 * VEIN_SCALE;
    int16_t left = veinLeft ? VEIN_MARGIN : w - VEIN_MARGIN - size;
    int16_t cx = left + size / 2;
    int16_t cy = VEIN_TOP + size / 2;

    // Pulse: 0..255..0 over VEIN_PULSE_MS
    unsigned long phase = now % VEIN_PULSE_MS;
    uint8_t pulse = (uint8_t)(phase < VEIN_PULSE_MS / 2 ? phase * 510 / VEIN_PULSE_MS : (VEIN_PULSE_MS - phase) * 510 / VEIN_PULSE_MS);
    sprite->fillCircle(cx, cy, size / 2 + 3, tft.alphaBlend(40 + pulse / 4, Theme::GOLD, eraseColor));

    for (int16_t row = 0; row < 16; row++)
    {
        for (int16_t col = 0; col < 16; col++)
        {
            uint16_t raw = pgm_read_word(veinIcon + row * 16 + col);
            if (raw != 0)
            {
                sprite->fillRect(left + col * VEIN_SCALE, VEIN_TOP + row * VEIN_SCALE, VEIN_SCALE, VEIN_SCALE,
                                 swapBytes(raw));
            }
        }
    }

    // The button that catches it, right under it
    ButtonBadge::drawCap(*sprite, cx, VEIN_TOP + size + 10, 6, ButtonBadge::UP, false);
}

void OreDisplay::drawChips()
{
    for (uint8_t i = 0; i < MAX_CHIPS; i++)
    {
        const Chip &chip = chips[i];
        if (!chip.active)
        {
            continue;
        }
        int32_t age = now - chip.start;
        int16_t cx = chip.x + (int32_t)chip.vx * age / 1000;
        int16_t cy = chip.y + (int32_t)chip.vy * age / 1000 + (int32_t)CHIP_GRAVITY * age / 1000 * age / 2000;
        int16_t size = age < (int32_t)CHIP_MS / 2 ? CHIP_SIZE : CHIP_SIZE - 1;
        sprite->fillRect(cx - size / 2, cy - size / 2, size, size, chip.color);
    }
}

// Ease-in along a quadratic arc, so a coin leaves the ore slowly and speeds into the card
void OreDisplay::drawCoins()
{
    for (uint8_t i = 0; i < MAX_COINS; i++)
    {
        const Coin &coin = coins[i];
        long age = (long)(now - coin.start);
        if (!coin.active || age < 0)
        {
            continue;
        }
        int32_t t = (int32_t)age * 256 / COIN_MS;
        t = t * t / 256;
        int32_t u = 256 - t;
        int16_t px = (u * u * coin.x0 + 2 * u * t * coin.cx + t * t * coin.x1) / 65536;
        int16_t py = (u * u * coin.y0 + 2 * u * t * coin.cy + t * t * coin.y1) / 65536;
        sprite->fillCircle(px, py, COIN_RADIUS, Theme::GOLD);
        sprite->drawCircle(px, py, COIN_RADIUS, Theme::GOLD_FRAME);
        sprite->drawPixel(px - 1, py - 1, Theme::TEXT);
    }
}

void OreDisplay::draw(TFT_eSPI &tft)
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
        else
        {
            // The TFT_eSPI constructor leaves the free-font pointer uninitialized; that only works for
            // zeroed globals, so a heap-allocated sprite must select the built-in font explicitly
            sprite->setTextFont(1);
        }
    }

    // Not enough memory for the sprite: still show the ore, just without animations
    if (sprite == nullptr)
    {
        tft.fillRect(x, y, w, h, eraseColor);
        tft.pushImage(x + oreX, y + oreY, oreW, oreH, (uint16_t *)orePixels);
        return;
    }

    sprite->fillSprite(eraseColor);
    drawBackdrop(tft);
    drawOre(oreX + shakeOffset());
    drawVein(tft);
    drawChips();
    drawCoins();

    sprite->setTextSize(2);
    sprite->setTextDatum(MC_DATUM);
    for (uint8_t i = 0; i < MAX_POPUPS; i++)
    {
        const Popup &popup = popups[i];
        if (!popup.active)
        {
            continue;
        }

        unsigned long age = now - popup.start;
        int16_t popupY = oreY + oreH / 2 - (int32_t)POPUP_RISE * age / POPUP_MS;

        uint16_t color = popup.color;
        unsigned long fadeStart = POPUP_MS - POPUP_FADE_MS;
        if (age > fadeStart)
        {
            uint8_t alpha = 255 - 255 * (age - fadeStart) / POPUP_FADE_MS;
            color = tft.alphaBlend(alpha, popup.color, eraseColor);
        }

        sprite->setTextColor(color);
        sprite->drawString(popup.text, popup.x, popupY);
    }

    sprite->pushSprite(x, y);
}
