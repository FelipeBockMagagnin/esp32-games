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
    sprite->pushImage(oreX + shakeOffset(), oreY, oreW, oreH, (uint16_t *)orePixels);

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
