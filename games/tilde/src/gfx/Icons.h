#ifndef GFX_ICONS_H
#define GFX_ICONS_H

#include "Canvas.h"

enum class Icon : uint8_t
{
    HEART,
    STAR,
    MOON,
    SPARKLE,
    DROP,
    NOTE,
    LEAF,
    CROSS,
    BOWL,
    CANDY,
    BALL,
    BROOM,
    BULB,
    ARROW_LEFT,
    ARROW_RIGHT,
    ARROW_UP,
    ARROW_DOWN,
    COUNT
};

// Drawn from shapes, centred on (cx, cy) in a box `size` wide. `detail` colours the inner marks
// (a ball's stripe, a candy's twist), usually the surface the icon sits on.
void drawIcon(Canvas &c, Icon icon, float cx, float cy, float size, uint16_t color, uint16_t detail,
              uint8_t alpha = 255);

#endif // GFX_ICONS_H
