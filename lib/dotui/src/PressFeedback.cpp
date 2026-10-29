#include "PressFeedback.h"

static const unsigned long FLASH_MS = 180;
static const unsigned long SHAKE_STEP_MS = 30;
static const int8_t SHAKE_OFFSETS[] = {-3, 3, -2, 2, -1, 1};
static const uint8_t SHAKE_STEPS = sizeof(SHAKE_OFFSETS);

void PressFeedback::flash(ListRow &row)
{
    if (flashing != nullptr && flashing != &row)
    {
        flashing->setFlash(false);
    }
    flashing = &row;
    flashStart = now;
    row.setFlash(true);
}

void PressFeedback::shake(UI &element)
{
    if (shaking != &element)
    {
        if (shaking != nullptr)
        {
            shaking->setPosition(homeX, shaking->y);
        }
        shaking = &element;
        homeX = element.x;
    }
    shakeStart = now;
}

void PressFeedback::update(unsigned long time)
{
    now = time;

    if (flashing != nullptr && now - flashStart >= FLASH_MS)
    {
        flashing->setFlash(false);
        flashing = nullptr;
    }

    if (shaking != nullptr)
    {
        uint8_t step = (now - shakeStart) / SHAKE_STEP_MS;
        if (step >= SHAKE_STEPS)
        {
            shaking->setPosition(homeX, shaking->y);
            shaking = nullptr;
        }
        else
        {
            shaking->setPosition(homeX + SHAKE_OFFSETS[step], shaking->y);
        }
    }
}
