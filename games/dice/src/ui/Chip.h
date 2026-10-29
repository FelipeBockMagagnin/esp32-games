#ifndef UI_CHIP_H
#define UI_CHIP_H

#include "UI.h"

// A number already drawn, as a small token; the latest one stands out
class Chip : public UI
{
public:
    static const int16_t W = 42;
    static const int16_t H = 22;

    Chip() : UI(0, 0, W, H) {}

    void set(int16_t newValue, bool newLatest)
    {
        if (value != newValue || latest != newLatest)
        {
            value = newValue;
            latest = newLatest;
            markDirty();
        }
    }

    void draw(TFT_eSPI &tft) override;

private:
    int16_t value = 0;
    bool latest = false;
};

#endif // UI_CHIP_H
