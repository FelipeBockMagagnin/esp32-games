#ifndef UI_LABELS_H
#define UI_LABELS_H

#include "Text.h"
#include "Theme.h"

// Text painted with its own background, so a changing number overwrites the last one
// without an erase in between
inline Text label(int16_t x, int16_t y, uint8_t datum = TL_DATUM, Color color = Theme::TEXT, uint8_t size = 1,
                  Color surface = Theme::FACE)
{
    return Text(x, y, "", color, size, datum, false, surface);
}

#endif // UI_LABELS_H
