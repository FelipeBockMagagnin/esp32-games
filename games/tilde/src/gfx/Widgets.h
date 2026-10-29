#ifndef GFX_WIDGETS_H
#define GFX_WIDGETS_H

#include "Canvas.h"
#include "Icons.h"
#include "Theme.h"

// The pieces every screen is made of, so they share one soft, rounded look
namespace Widgets
{
    // A card with a soft shadow under it. Only the shadow's bottom strip is drawn: the card
    // covers the rest, and blending what is never seen is most of a frame's cost.
    inline void card(Canvas &c, float x, float y, float w, float h, float r = 12, uint8_t color = CARD)
    {
        if (!c.reaches(y - 1, y + h + 3))
        {
            return;
        }
        float strip = r + 2;
        c.fillRoundRect(x, y + h + 2 - strip * 2, w, strip * 2, r, Theme::rgb(SHADOW), 90);
        c.fillRoundRect(x, y, w, h, r, Theme::rgb(color));
    }

    // A pill that fills from the left; `value` 0..1
    inline void meter(Canvas &c, float x, float y, float w, float h, float value, uint8_t color)
    {
        uint16_t fill = Theme::rgb(color);
        c.fillRoundRect(x, y, w, h, h / 2, Canvas::mix(fill, Theme::rgb(CARD), 190));
        if (value > 0.02f)
        {
            float fw = h + (w - h) * (value > 1 ? 1 : value);
            c.fillRoundRect(x, y, fw, h, h / 2, fill);
            c.fillRoundRect(x + h * 0.3f, y + h * 0.18f, fw - h * 0.6f, h * 0.22f, h * 0.11f, Theme::rgb(SHINE), 110);
        }
    }

    inline void hint(Canvas &c, float y, const char *text)
    {
        c.text(Canvas::W / 2, y, text, FONT_SMALL, Theme::rgb(INK_FAINT), Canvas::CENTER);
    }

    // A round button; `pop` above 1 while it springs
    inline void button(Canvas &c, float cx, float cy, float r, Icon icon, bool selected, bool open, float pop = 1)
    {
        uint8_t alpha = open ? 255 : 120;
        float rr = r * pop;
        c.fillCircle(cx, cy + 2, rr, Theme::rgb(SHADOW), 90);
        uint16_t fill = Theme::rgb(selected ? (open ? ACCENT : INK_FAINT) : CARD);
        c.fillCircle(cx, cy, rr, fill, selected ? 255 : alpha);
        uint16_t ink = Theme::rgb(selected ? ACCENT_INK : INK_SOFT);
        drawIcon(c, icon, cx, cy, rr * 1.15f, ink, fill, selected ? 255 : alpha);
    }
}

#endif // GFX_WIDGETS_H
