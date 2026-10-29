#include "Icons.h"
#include <math.h>

static void star(Canvas &c, float cx, float cy, float r, float inner, uint8_t points, uint16_t color, uint8_t alpha)
{
    const float step = (float)M_PI / points;
    c.fillCircle(cx, cy, inner * 1.02f, color, alpha);
    for (uint8_t i = 0; i < points; i++)
    {
        float a = -(float)M_PI / 2 + i * 2 * step;
        c.fillTriangle(cx + cosf(a) * r, cy + sinf(a) * r, cx + cosf(a - step) * inner, cy + sinf(a - step) * inner,
                       cx + cosf(a + step) * inner, cy + sinf(a + step) * inner, color, alpha);
    }
}

void drawIcon(Canvas &c, Icon icon, float cx, float cy, float s, uint16_t color, uint16_t detail, uint8_t alpha)
{
    if (!c.reaches(cy - s, cy + s))
    {
        return;
    }
    switch (icon)
    {
    case Icon::HEART:
        c.fillCircle(cx - s * 0.22f, cy - s * 0.1f, s * 0.26f, color, alpha);
        c.fillCircle(cx + s * 0.22f, cy - s * 0.1f, s * 0.26f, color, alpha);
        c.fillTriangle(cx - s * 0.46f, cy - s * 0.02f, cx + s * 0.46f, cy - s * 0.02f, cx, cy + s * 0.42f, color, alpha);
        c.fillCircle(cx - s * 0.24f, cy - s * 0.16f, s * 0.07f, detail, alpha / 2);
        break;
    case Icon::STAR:
        star(c, cx, cy + s * 0.04f, s * 0.48f, s * 0.22f, 5, color, alpha);
        break;
    case Icon::SPARKLE:
        star(c, cx, cy, s * 0.48f, s * 0.12f, 4, color, alpha);
        star(c, cx + s * 0.32f, cy - s * 0.3f, s * 0.16f, s * 0.05f, 4, color, alpha);
        break;
    case Icon::MOON:
        c.arc(cx, cy, s * 0.26f, s * 0.24f, 1.9f, 5.6f, color, alpha);
        break;
    case Icon::DROP:
        c.fillCircle(cx, cy + s * 0.12f, s * 0.28f, color, alpha);
        c.fillTriangle(cx - s * 0.25f, cy, cx + s * 0.25f, cy, cx, cy - s * 0.44f, color, alpha);
        break;
    case Icon::NOTE:
        c.fillEllipse(cx - s * 0.12f, cy + s * 0.26f, s * 0.18f, s * 0.14f, color, alpha);
        c.line(cx + s * 0.04f, cy + s * 0.24f, cx + s * 0.04f, cy - s * 0.38f, s * 0.09f, color, alpha);
        c.line(cx + s * 0.04f, cy - s * 0.38f, cx + s * 0.3f, cy - s * 0.2f, s * 0.09f, color, alpha);
        break;
    case Icon::LEAF:
        c.fillEllipse(cx, cy - s * 0.06f, s * 0.24f, s * 0.38f, color, alpha);
        c.line(cx, cy - s * 0.3f, cx, cy + s * 0.44f, s * 0.07f, detail, alpha);
        break;
    case Icon::CROSS:
        c.fillRoundRect(cx - s * 0.13f, cy - s * 0.42f, s * 0.26f, s * 0.84f, s * 0.08f, color, alpha);
        c.fillRoundRect(cx - s * 0.42f, cy - s * 0.13f, s * 0.84f, s * 0.26f, s * 0.08f, color, alpha);
        break;
    case Icon::BOWL:
        c.fillCircle(cx - s * 0.14f, cy - s * 0.08f, s * 0.15f, detail, alpha);
        c.fillCircle(cx + s * 0.12f, cy - s * 0.1f, s * 0.17f, detail, alpha);
        c.fillRoundRect(cx - s * 0.46f, cy - s * 0.04f, s * 0.92f, s * 0.42f, s * 0.2f, color, alpha);
        c.fillRoundRect(cx - s * 0.5f, cy - s * 0.08f, s, s * 0.1f, s * 0.05f, color, alpha);
        break;
    case Icon::CANDY:
        c.fillTriangle(cx - s * 0.2f, cy, cx - s * 0.5f, cy - s * 0.2f, cx - s * 0.5f, cy + s * 0.2f, color, alpha);
        c.fillTriangle(cx + s * 0.2f, cy, cx + s * 0.5f, cy - s * 0.2f, cx + s * 0.5f, cy + s * 0.2f, color, alpha);
        c.fillCircle(cx, cy, s * 0.26f, color, alpha);
        c.arc(cx, cy, s * 0.13f, s * 0.07f, 3.4f, 6.0f, detail, alpha);
        break;
    case Icon::BALL:
        c.fillCircle(cx, cy, s * 0.42f, color, alpha);
        c.arc(cx, cy + s * 0.5f, s * 0.55f, s * 0.08f, 3.7f, 5.7f, detail, alpha);
        c.fillCircle(cx - s * 0.16f, cy - s * 0.18f, s * 0.08f, detail, alpha / 2);
        break;
    case Icon::BROOM:
        c.line(cx + s * 0.3f, cy - s * 0.44f, cx - s * 0.05f, cy + s * 0.08f, s * 0.1f, color,
               alpha);
        c.fillTriangle(cx - s * 0.02f, cy + s * 0.02f, cx - s * 0.38f, cy + s * 0.44f, cx + s * 0.12f, cy + s * 0.44f,
                       color, alpha);
        break;
    case Icon::BULB:
        c.fillCircle(cx, cy - s * 0.08f, s * 0.3f, color, alpha);
        c.fillRoundRect(cx - s * 0.14f, cy + s * 0.16f, s * 0.28f, s * 0.26f, s * 0.06f, color, alpha);
        c.line(cx - s * 0.12f, cy + s * 0.26f, cx + s * 0.12f, cy + s * 0.26f, s * 0.05f, detail, alpha);
        c.fillCircle(cx - s * 0.1f, cy - s * 0.16f, s * 0.07f, detail, alpha / 2);
        break;
    case Icon::ARROW_LEFT:
        c.fillTriangle(cx - s * 0.32f, cy, cx + s * 0.22f, cy - s * 0.34f, cx + s * 0.22f, cy + s * 0.34f, color, alpha);
        break;
    case Icon::ARROW_RIGHT:
        c.fillTriangle(cx + s * 0.32f, cy, cx - s * 0.22f, cy - s * 0.34f, cx - s * 0.22f, cy + s * 0.34f, color, alpha);
        break;
    case Icon::ARROW_UP:
        c.fillTriangle(cx, cy - s * 0.32f, cx - s * 0.34f, cy + s * 0.22f, cx + s * 0.34f, cy + s * 0.22f, color, alpha);
        break;
    case Icon::ARROW_DOWN:
        c.fillTriangle(cx, cy + s * 0.32f, cx - s * 0.34f, cy - s * 0.22f, cx + s * 0.34f, cy - s * 0.22f, color, alpha);
        break;
    default:
        break;
    }
}
