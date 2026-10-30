#ifndef UI_PET_DRAW_H
#define UI_PET_DRAW_H

#include "LcdMatrix.h"

inline void drawHearts(LcdMatrix &lcd, uint8_t filled, uint8_t max)
{
    for (uint8_t i = 0; i < max; i++)
    {
        lcd.sprite(i < filled ? ART_HEART : ART_HEART_EMPTY, 1 + i * 8, 9);
    }
}

#endif // UI_PET_DRAW_H
