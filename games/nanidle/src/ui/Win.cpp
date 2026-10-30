#include "Win.h"
#include "Themes.h"

// The Windows flag of the Start button: art, not theme colours
static const uint16_t FLAG_COLORS[4] = {rgb(0xf35325), rgb(0x81bc06), rgb(0x05a6f0), rgb(0xffba08)};

static WinStyle style()
{
    return activeTheme().style;
}

static void boldString(TFT_eSPI &tft, const char *text, int16_t x, int16_t y)
{
    tft.drawString(text, x, y);
    tft.drawString(text, x + 1, y);
}

namespace Win
{
    void gradientH(TFT_eSPI &tft, int16_t x, int16_t y, int16_t w, int16_t h, uint16_t from, uint16_t to)
    {
        for (int16_t i = 0; i < w; i++)
        {
            uint8_t alpha = w > 1 ? (uint8_t)(255 * i / (w - 1)) : 0;
            tft.drawFastVLine(x + i, y, h, tft.alphaBlend(alpha, to, from));
        }
    }

    void gradientV(TFT_eSPI &tft, int16_t x, int16_t y, int16_t w, int16_t h, uint16_t from, uint16_t to)
    {
        for (int16_t i = 0; i < h; i++)
        {
            uint8_t alpha = h > 1 ? (uint8_t)(255 * i / (h - 1)) : 0;
            tft.drawFastHLine(x, y + i, w, tft.alphaBlend(alpha, to, from));
        }
    }

    void bevel(TFT_eSPI &tft, int16_t x, int16_t y, int16_t w, int16_t h, bool raised)
    {
        if (style() == WinStyle::CLASSIC)
        {
            uint16_t outerLit = raised ? Theme::EDGE_HI : Theme::EDGE_LO;
            uint16_t outerDark = raised ? Theme::EDGE_DARK : Theme::EDGE_HI;
            uint16_t innerLit = raised ? Theme::FACE : Theme::EDGE_DARK;
            uint16_t innerDark = raised ? Theme::EDGE_LO : Theme::FACE;
            tft.drawFastHLine(x, y, w - 1, outerLit);
            tft.drawFastVLine(x, y, h - 1, outerLit);
            tft.drawFastHLine(x, y + h - 1, w, outerDark);
            tft.drawFastVLine(x + w - 1, y, h, outerDark);
            tft.drawFastHLine(x + 1, y + 1, w - 3, innerLit);
            tft.drawFastVLine(x + 1, y + 1, h - 3, innerLit);
            tft.drawFastHLine(x + 1, y + h - 2, w - 2, innerDark);
            tft.drawFastVLine(x + w - 2, y + 1, h - 2, innerDark);
            return;
        }
        if (raised)
        {
            tft.drawRoundRect(x, y, w, h, 3, Theme::EDGE_LO);
        }
        else
        {
            tft.drawRect(x, y, w, h, Theme::XP_BORDER);
        }
    }

    void field(TFT_eSPI &tft, int16_t x, int16_t y, int16_t w, int16_t h)
    {
        bevel(tft, x, y, w, h, false);
        int16_t inset = style() == WinStyle::CLASSIC ? 2 : 1;
        tft.fillRect(x + inset, y + inset, w - 2 * inset, h - 2 * inset, Theme::FIELD);
    }

    void button(TFT_eSPI &tft, int16_t x, int16_t y, int16_t w, int16_t h, const char *label, bool focused,
                bool enabled)
    {
        WinStyle s = style();
        if (s == WinStyle::CLASSIC)
        {
            tft.fillRect(x, y, w, h, Theme::FACE);
            if (focused)
            {
                tft.drawRect(x, y, w, h, Theme::EDGE_DARK);
                bevel(tft, x + 1, y + 1, w - 2, h - 2, true);
                focus(tft, x + 4, y + 4, w - 8, h - 8, true);
            }
            else
            {
                bevel(tft, x, y, w, h, true);
            }
        }
        else
        {
            tft.fillRect(x, y, w, h, Theme::FACE);
            uint16_t top = Theme::FIELD;
            uint16_t bottom = s == WinStyle::LUNA ? (uint16_t)Theme::FACE : tft.alphaBlend(200, Theme::FACE, Theme::EDGE_LO);
            gradientV(tft, x + 1, y + 1, w - 2, h - 2, top, bottom);
            tft.drawRoundRect(x, y, w, h, 3, s == WinStyle::LUNA ? Theme::EDGE_DARK : Theme::EDGE_LO);
            if (focused)
            {
                tft.drawRoundRect(x + 1, y + 1, w - 2, h - 2, 2, Theme::SELECT);
                tft.drawRoundRect(x + 2, y + 2, w - 4, h - 4, 2, tft.alphaBlend(120, Theme::SELECT, Theme::FIELD));
            }
        }

        tft.setTextDatum(MC_DATUM);
        tft.setTextSize(1);
        int16_t cx = x + w / 2;
        int16_t cy = y + h / 2 + 1;
        if (!enabled && s == WinStyle::CLASSIC)
        {
            tft.setTextColor(Theme::EDGE_HI);
            tft.drawString(label, cx + 1, cy + 1);
        }
        tft.setTextColor(enabled ? Theme::TEXT : Theme::TEXT_DIM);
        tft.drawString(label, cx, cy);
        tft.setTextDatum(TL_DATUM);
    }

    static void captionButton(TFT_eSPI &tft, int16_t x, int16_t y, int16_t w, int16_t h, char glyph)
    {
        WinStyle s = style();
        uint16_t ink;
        if (s == WinStyle::CLASSIC)
        {
            tft.fillRect(x, y, w, h, Theme::FACE);
            bevel(tft, x, y, w, h, true);
            ink = Theme::TEXT;
        }
        else
        {
            uint16_t base = glyph == 'x' ? (uint16_t)Theme::CLOSE
                                         : (s == WinStyle::LUNA ? (uint16_t)Theme::TITLE_A : (uint16_t)Theme::TITLE_B);
            gradientV(tft, x, y, w, h, tft.alphaBlend(110, Theme::FIELD, base), base);
            tft.drawRoundRect(x, y, w, h, 2, s == WinStyle::LUNA ? Theme::FIELD : Theme::EDGE_DARK);
            ink = Theme::FIELD;
        }

        int16_t cx = x + w / 2;
        int16_t cy = y + h / 2;
        switch (glyph)
        {
        case '_':
            tft.fillRect(cx - 3, cy + 2, 6, 2, ink);
            break;
        case 'o':
            tft.drawRect(cx - 4, cy - 3, 8, 7, ink);
            tft.drawFastHLine(cx - 4, cy - 2, 8, ink);
            break;
        default:
            for (int16_t i = -3; i <= 3; i++)
            {
                tft.drawPixel(cx + i, cy + i, ink);
                tft.drawPixel(cx + i + 1, cy + i, ink);
                tft.drawPixel(cx + i, cy - i, ink);
                tft.drawPixel(cx + i + 1, cy - i, ink);
            }
            break;
        }
    }

    void window(TFT_eSPI &tft, const char *title)
    {
        WinStyle s = style();
        int16_t w = DISPLAY_W;
        int16_t h = DISPLAY_H - TASKBAR_H;

        if (s == WinStyle::CLASSIC)
        {
            tft.fillRect(0, 0, w, BODY_Y, Theme::FACE);
            bevel(tft, 0, 0, w, h, true);
            tft.drawFastVLine(2, 2, h - 4, Theme::FACE);
            tft.drawFastVLine(w - 3, 2, h - 4, Theme::FACE);
            tft.drawFastHLine(2, h - 3, w - 4, Theme::FACE);
            gradientH(tft, 3, 3, w - 6, TITLE_H - 5, Theme::TITLE_A, Theme::TITLE_B);
        }
        else
        {
            uint16_t edge = s == WinStyle::LUNA ? (uint16_t)Theme::TITLE_B : (uint16_t)Theme::TITLE_B;
            tft.fillRect(0, 0, w, h, Theme::DESKTOP);
            tft.fillRoundRect(0, 0, w, TITLE_H + 8, 7, edge);
            tft.fillRect(0, TITLE_H, w, h - TITLE_H, edge);
            if (s == WinStyle::LUNA)
            {
                gradientV(tft, 3, 1, w - 6, 4, tft.alphaBlend(160, Theme::FIELD, Theme::TITLE_A), Theme::TITLE_A);
                gradientV(tft, 2, 5, w - 4, TITLE_H - 5, Theme::TITLE_A, Theme::TITLE_B);
            }
            else
            {
                gradientV(tft, 3, 1, w - 6, TITLE_H / 2, Theme::TITLE_A, tft.alphaBlend(200, Theme::TITLE_A, Theme::TITLE_B));
                gradientV(tft, 2, TITLE_H / 2 + 1, w - 4, TITLE_H / 2 - 1, Theme::TITLE_B, Theme::TITLE_A);
                tft.drawRoundRect(0, 0, w, h, 6, Theme::EDGE_DARK);
            }
            tft.drawRect(BODY_X - 1, BODY_Y - 1, BODY_W + 2, BODY_BOTTOM - BODY_Y + 2,
                         s == WinStyle::LUNA ? Theme::TITLE_B : Theme::EDGE_DARK);
        }
        tft.fillRect(BODY_X, BODY_Y, BODY_W, BODY_BOTTOM - BODY_Y, Theme::FACE);

        // The little application icon, then the title
        int16_t iconY = s == WinStyle::CLASSIC ? 5 : 4;
        tft.fillRect(6, iconY, 11, 11, Theme::FIELD);
        tft.drawRect(6, iconY, 11, 11, Theme::EDGE_DARK);
        tft.setTextSize(1);
        tft.setTextDatum(TL_DATUM);
        tft.setTextColor(Theme::BAD);
        tft.drawString("N", 9, iconY + 2);
        tft.setTextColor(Theme::TITLE_TEXT);
        boldString(tft, title, 21, iconY + 2);

        // Caption buttons
        if (s == WinStyle::CLASSIC)
        {
            captionButton(tft, w - 51, 5, 14, 12, '_');
            captionButton(tft, w - 37, 5, 14, 12, 'o');
            captionButton(tft, w - 21, 5, 14, 12, 'x');
        }
        else if (s == WinStyle::LUNA)
        {
            captionButton(tft, w - 54, 3, 15, 15, '_');
            captionButton(tft, w - 37, 3, 15, 15, 'o');
            captionButton(tft, w - 20, 3, 15, 15, 'x');
        }
        else
        {
            captionButton(tft, w - 70, 2, 20, 13, '_');
            captionButton(tft, w - 50, 2, 20, 13, 'o');
            captionButton(tft, w - 30, 2, 26, 13, 'x');
        }
    }

    void bar(TFT_eSPI &tft, int16_t x, int16_t y, int16_t w, int16_t h, int16_t filled, Color fill, Color hi)
    {
        WinStyle s = style();
        int16_t inset = s == WinStyle::CLASSIC ? 2 : 1;
        bevel(tft, x, y, w, h, false);
        int16_t ix = x + inset, iy = y + inset, iw = w - 2 * inset, ih = h - 2 * inset;
        if (filled > iw)
        {
            filled = iw;
        }

        if (s == WinStyle::AERO)
        {
            if (filled > 0)
            {
                gradientV(tft, ix, iy, filled, ih / 2, hi, fill);
                tft.fillRect(ix, iy + ih / 2, filled, ih - ih / 2, fill);
            }
            if (filled < iw)
            {
                gradientV(tft, ix + filled, iy, iw - filled, ih, Theme::FIELD, Theme::XP_TRACK);
            }
            return;
        }

        tft.fillRect(ix, iy, iw, ih, Theme::FIELD);
        // Blocks with a gap, the last one cut where the value ends
        const int16_t block = s == WinStyle::CLASSIC ? 7 : 6;
        const int16_t gap = 2;
        for (int16_t bx = 1; bx < filled; bx += block + gap)
        {
            int16_t bw = filled - bx < block ? filled - bx : block;
            if (s == WinStyle::LUNA)
            {
                gradientV(tft, ix + bx, iy + 1, bw, ih - 2, hi, fill);
            }
            else
            {
                tft.fillRect(ix + bx, iy + 1, bw, ih - 2, fill);
            }
        }
    }

    void focus(TFT_eSPI &tft, int16_t x, int16_t y, int16_t w, int16_t h, bool on)
    {
        if (style() == WinStyle::CLASSIC)
        {
            uint16_t ink = on ? (uint16_t)Theme::TEXT : (uint16_t)Theme::FACE;
            uint16_t face = Theme::FACE;
            for (int16_t i = 0; i < w; i++)
            {
                uint16_t c = (i & 1) == 0 ? ink : face;
                tft.drawPixel(x + i, y, c);
                tft.drawPixel(x + i, y + h - 1, c);
            }
            for (int16_t j = 0; j < h; j++)
            {
                uint16_t c = (j & 1) == 0 ? ink : face;
                tft.drawPixel(x, y + j, c);
                tft.drawPixel(x + w - 1, y + j, c);
            }
            return;
        }
        uint16_t outer = on ? (uint16_t)Theme::SELECT : (uint16_t)Theme::FACE;
        uint16_t inner = on ? tft.alphaBlend(110, Theme::SELECT, Theme::FACE) : (uint16_t)Theme::FACE;
        tft.drawRoundRect(x, y, w, h, 3, outer);
        tft.drawRoundRect(x + 1, y + 1, w - 2, h - 2, 2, inner);
    }
}

// ---- Taskbar --------------------------------------------------------------------------------

static void copyText(char *dst, size_t size, const char *src)
{
    strncpy(dst, src != nullptr ? src : "", size - 1);
    dst[size - 1] = '\0';
}

void Taskbar::setNav(const char *newBack, const char *newNext)
{
    if (strcmp(back, newBack) != 0 || strcmp(next, newNext) != 0)
    {
        copyText(back, sizeof(back), newBack);
        copyText(next, sizeof(next), newNext);
        markDirty();
    }
}

void Taskbar::setClock(const char *text)
{
    if (strcmp(clock, text) != 0)
    {
        copyText(clock, sizeof(clock), text);
        markDirty();
    }
}

void Taskbar::setNotice(const char *text, unsigned long until)
{
    copyText(notice, sizeof(notice), text);
    noticeUntil = until;
    flash = true;
    markDirty();
}

void Taskbar::update(unsigned long time)
{
    now = time;
    if (notice[0] == '\0')
    {
        return;
    }
    if ((long)(now - noticeUntil) >= 0)
    {
        notice[0] = '\0';
        markDirty();
        return;
    }
    bool on = (noticeUntil - now) / 400 % 2 == 0;
    if (on != flash)
    {
        flash = on;
        markDirty();
    }
}

static void taskButton(TFT_eSPI &tft, int16_t x, int16_t y, int16_t w, int16_t h, const char *label, bool lit)
{
    WinStyle s = style();
    if (s == WinStyle::CLASSIC)
    {
        tft.fillRect(x, y, w, h, lit ? Theme::HILITE : Theme::FACE);
        Win::bevel(tft, x, y, w, h, true);
        tft.setTextColor(lit ? Theme::HILITE_TEXT : Theme::TEXT);
    }
    else
    {
        uint16_t base = lit ? (uint16_t)Theme::START : (uint16_t)Theme::TASKBAR_HI;
        Win::gradientV(tft, x, y, w, h, tft.alphaBlend(90, Theme::FIELD, base), base);
        tft.drawRoundRect(x, y, w, h, 2, s == WinStyle::LUNA ? Theme::TITLE_B : Theme::EDGE_DARK);
        tft.setTextColor(Theme::FIELD);
    }
    tft.setTextDatum(MC_DATUM);
    tft.drawString(label, x + w / 2, y + h / 2 + 1);
    tft.setTextDatum(TL_DATUM);
}

void Taskbar::draw(TFT_eSPI &tft)
{
    WinStyle s = style();
    tft.setTextSize(1);
    if (s == WinStyle::CLASSIC)
    {
        tft.fillRect(x, y, w, h, Theme::TASKBAR);
        tft.drawFastHLine(x, y + 1, w, Theme::TASKBAR_HI);
    }
    else
    {
        Win::gradientV(tft, x, y, w, h, Theme::TASKBAR_HI, Theme::TASKBAR);
    }

    // Start
    int16_t startW = 60;
    if (s == WinStyle::CLASSIC)
    {
        tft.fillRect(x + 2, y + 3, startW, h - 5, Theme::FACE);
        Win::bevel(tft, x + 2, y + 3, startW, h - 5, true);
    }
    else if (s == WinStyle::LUNA)
    {
        tft.fillRoundRect(x - 8, y, startW + 10, h, 8, Theme::START);
        Win::gradientV(tft, x, y + 1, startW - 4, h / 2, Theme::START_HI, Theme::START);
    }
    else
    {
        tft.fillCircle(x + 11, y + h / 2, 9, Theme::START);
        tft.drawCircle(x + 11, y + h / 2, 9, Theme::START_HI);
        startW = 22;
    }
    int16_t fx = s == WinStyle::AERO ? x + 7 : x + 6;
    int16_t fy = y + h / 2 - 4;
    for (uint8_t i = 0; i < 4; i++)
    {
        tft.fillRect(fx + (i % 2) * 4, fy + (i / 2) * 4, 3, 3, FLAG_COLORS[i]);
    }
    if (s != WinStyle::AERO)
    {
        tft.setTextColor(Theme::START_TEXT);
        tft.setTextDatum(ML_DATUM);
        boldString(tft, s == WinStyle::LUNA ? "iniciar" : "Iniciar", fx + 10, y + h / 2 + 1);
        tft.setTextDatum(TL_DATUM);
    }

    // Tray
    int16_t trayW = 40;
    int16_t trayX = x + w - trayW - 2;
    if (s == WinStyle::CLASSIC)
    {
        tft.fillRect(trayX, y + 3, trayW, h - 5, Theme::FACE);
        Win::bevel(tft, trayX, y + 3, trayW, h - 5, false);
        tft.setTextColor(Theme::TEXT);
    }
    else
    {
        tft.fillRect(trayX, y + 1, trayW + 2, h - 1, Theme::TRAY);
        tft.drawFastVLine(trayX, y + 1, h - 1, Theme::TASKBAR_HI);
        tft.setTextColor(Theme::FIELD);
    }
    tft.setTextDatum(MC_DATUM);
    tft.drawString(clock, trayX + trayW / 2, y + h / 2 + 1);
    tft.setTextDatum(TL_DATUM);

    // Tasks: the screens either side, or the notice
    int16_t tasksX = x + startW + 6;
    int16_t tasksW = trayX - tasksX - 3;
    if (notice[0] != '\0')
    {
        taskButton(tft, tasksX, y + 3, tasksW, h - 5, notice, flash);
        return;
    }
    char label[16];
    int16_t half = (tasksW - 3) / 2;
    if (back[0] != '\0')
    {
        snprintf(label, sizeof(label), "<%s", back);
        taskButton(tft, tasksX, y + 3, half, h - 5, label, false);
    }
    if (next[0] != '\0')
    {
        snprintf(label, sizeof(label), "%s>", next);
        taskButton(tft, tasksX + half + 3, y + 3, half, h - 5, label, false);
    }
}

// ---- Bars, buttons, panels ------------------------------------------------------------------

void WinBar::setValue(double fraction)
{
    if (!(fraction > 0))
    {
        fraction = 0;
    }
    if (fraction > 1)
    {
        fraction = 1;
    }
    int16_t inner = w - (style() == WinStyle::CLASSIC ? 4 : 2);
    int16_t px = (int16_t)(fraction * inner);
    if (px != filled)
    {
        filled = px;
        markDirty();
    }
}

void WinBar::setColors(Color newFill, Color newHi)
{
    if (fill != newFill || hi != newHi)
    {
        fill = newFill;
        hi = newHi;
        markDirty();
    }
}

void WinBar::setLabel(const char *text)
{
    if (strcmp(label, text) != 0)
    {
        copyText(label, sizeof(label), text);
        markDirty();
    }
}

void WinBar::draw(TFT_eSPI &tft)
{
    Win::bar(tft, x, y, w, h, filled, fill, hi);
    if (label[0] == '\0')
    {
        return;
    }
    // Dark with a light outline, readable over the blocks, the gaps and the track alike
    tft.setTextSize(1);
    tft.setTextDatum(MC_DATUM);
    int16_t cx = x + w / 2;
    int16_t cy = y + h / 2 + 1;
    tft.setTextColor(Theme::FIELD);
    for (int8_t dx = -1; dx <= 1; dx++)
    {
        for (int8_t dy = -1; dy <= 1; dy++)
        {
            if (dx != 0 || dy != 0)
            {
                tft.drawString(label, cx + dx, cy + dy);
            }
        }
    }
    tft.setTextColor(Theme::TEXT);
    tft.drawString(label, cx, cy);
    tft.setTextDatum(TL_DATUM);
}

void WinButton::setLabel(const char *text)
{
    if (strcmp(label, text) != 0)
    {
        copyText(label, sizeof(label), text);
        markDirty();
    }
}

void WinButton::setFocused(bool on)
{
    if (focused != on)
    {
        focused = on;
        markDirty();
    }
}

void WinButton::setEnabled(bool on)
{
    if (enabled != on)
    {
        enabled = on;
        markDirty();
    }
}

void WinPanel::draw(TFT_eSPI &tft)
{
    if (filled)
    {
        Win::field(tft, x, y, w, h);
    }
    else
    {
        Win::bevel(tft, x, y, w, h, raised);
    }
}

void Icon16::draw(TFT_eSPI &tft)
{
    tft.fillRect(x, y, w, h, eraseColor);
    if (pixels != nullptr)
    {
        tft.pushImage(x, y, 16, 16, pixels, 0x0000);
    }
}
