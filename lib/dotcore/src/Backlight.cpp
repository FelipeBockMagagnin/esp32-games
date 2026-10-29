#include "Backlight.h"

void Backlight::begin()
{
    ledcSetup(CHANNEL, 5000, 8);
    ledcAttachPin(pin, CHANNEL);
}

void Backlight::setFull()
{
    dimmed = false;
    ledcWrite(CHANNEL, FULL);
}

bool Backlight::update(unsigned long now, bool pressed, bool held, unsigned long dimAfter)
{
    if (pressed || held)
    {
        lastInputAt = now;
    }
    if (dimmed && pressed)
    {
        setFull();
        return true;
    }
    if (!dimmed && dimAfter > 0 && now - lastInputAt >= dimAfter)
    {
        dimmed = true;
        ledcWrite(CHANNEL, DIM);
    }
    return false;
}
