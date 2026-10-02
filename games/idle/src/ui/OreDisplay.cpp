#include "OreDisplay.h"

// ~30 fps is smooth enough and leaves the SPI bus free for the rest of the screen
static const unsigned long FRAME_MS = 33;

static const unsigned long SHAKE_STEP_MS = 20;
static const int8_t SHAKE_OFFSETS[] = {-4, 4, -3, 3, -2, 2, -1, 1};
static const uint8_t SHAKE_STEPS = sizeof(SHAKE_OFFSETS) / sizeof(SHAKE_OFFSETS[0]);

static const unsigned long POPUP_MS = 700;
static const int16_t POPUP_RISE = 45;          // Pixels travelled over its lifetime
static const unsigned long POPUP_FADE_MS = 300; // Fades out during the last part
static const int16_t POPUP_SPREAD = 20;         // Random horizontal jitter so fast clicks don't stack

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
      shaking(false),
      shakeStart(0),
      now(0),
      lastFrame(0),
      animating(false),
      sprite(nullptr)
{
    for (uint8_t i = 0; i < MAX_POPUPS; i++)
    {
        popups[i].active = false;
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
    // Reuses the oldest slot when all are in use
    Popup &popup = popups[nextPopup];
    nextPopup = (nextPopup + 1) % MAX_POPUPS;

    popup.active = true;
    popup.start = now;
    popup.x = oreX + oreW / 2 + random(-POPUP_SPREAD, POPUP_SPREAD + 1);
    popup.text = text;
    lastFrame = 0;
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

void OreDisplay::update(unsigned long time)
{
    now = time;

    if (shaking && now - shakeStart >= SHAKE_STEP_MS * SHAKE_STEPS)
    {
        shaking = false;
    }

    bool anyPopup = false;
    for (uint8_t i = 0; i < MAX_POPUPS; i++)
    {
        if (popups[i].active && now - popups[i].start >= POPUP_MS)
        {
            popups[i].active = false;
        }
        anyPopup = anyPopup || popups[i].active;
    }

    bool wasAnimating = animating;
    animating = shaking || anyPopup;

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

        uint16_t color = popupColor;
        unsigned long fadeStart = POPUP_MS - POPUP_FADE_MS;
        if (age > fadeStart)
        {
            uint8_t alpha = 255 - 255 * (age - fadeStart) / POPUP_FADE_MS;
            color = tft.alphaBlend(alpha, popupColor, eraseColor);
        }

        sprite->setTextColor(color);
        sprite->drawString(popup.text, popup.x, popupY);
    }

    sprite->pushSprite(x, y);
}
