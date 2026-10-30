#ifndef UI_BATTERY_H
#define UI_BATTERY_H

#include "UI.h"

// The web game's energy battery: the case, its nub, the energy held split into what is
// allocated and what is idle, and the idle amount written over it
class Battery : public UI
{
public:
    Battery(int16_t x, int16_t y, int16_t w, int16_t h) : UI(x, y, w, h) {}

    // Fractions of the cap
    void setLevels(double allocated, double idle);
    void setLabel(const char *text);
    void draw(TFT_eSPI &tft) override;

private:
    int16_t allocatedPx = 0;
    int16_t idlePx = 0;
    char label[28] = "";
};

#endif // UI_BATTERY_H
