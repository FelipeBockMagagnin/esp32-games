#include "LcdPanel.h"
#include <string.h>
#include "Theme.h"

static const int16_t TITLE_CHARS = 18; // Size 2 glyphs across the glass
static const unsigned long SCROLL_MS = 220;
static const unsigned long HOLD_MS = 1500;

static bool copy(char *dst, size_t n, const char *src)
{
    if (strncmp(dst, src, n - 1) == 0)
    {
        return false;
    }
    strncpy(dst, src, n - 1);
    dst[n - 1] = '\0';
    return true;
}

void LcdPanel::set(const char *t, const char *c, const char *s, bool p)
{
    if (copy(title, sizeof(title), t))
    {
        scroll = 0;
        nextStep = 0;
        dirty = true;
    }
    dirty |= copy(credit, sizeof(credit), c);
    dirty |= copy(status, sizeof(status), s);
    if (p != playing)
    {
        playing = p;
        dirty = true;
    }
}

void LcdPanel::draw(TFT_eSPI &tft, unsigned long now)
{
    int16_t len = (int16_t)strlen(title);
    if (len > TITLE_CHARS && (long)(now - nextStep) >= 0)
    {
        bool fresh = nextStep == 0;
        if (!fresh)
        {
            scroll = (int16_t)((scroll + 1) % (len + 4));
            dirty = true;
        }
        nextStep = now + (scroll == 0 ? HOLD_MS : SCROLL_MS);
    }
    if (!dirty)
    {
        return;
    }
    dirty = false;

    TFT_eSprite glass(&tft);
    glass.setColorDepth(16);
    if (glass.createSprite(W, H) == nullptr)
    {
        return;
    }
    glass.fillSprite(Theme::LCD_BG);
    glass.drawRect(0, 0, W, H, Theme::LCD_DIM);

    char shown[TITLE_CHARS + 1];
    if (len > TITLE_CHARS)
    {
        for (int16_t i = 0; i < TITLE_CHARS; i++)
        {
            int16_t k = (scroll + i) % (len + 4);
            shown[i] = k < len ? title[k] : ' ';
        }
        shown[TITLE_CHARS] = '\0';
    }
    else
    {
        strncpy(shown, title, sizeof(shown));
        shown[TITLE_CHARS] = '\0';
    }
    glass.setTextFont(1);
    glass.setTextDatum(TL_DATUM);
    glass.setTextSize(2);
    glass.setTextColor(Theme::LCD_DIM);
    glass.drawString("888888888888888888", 7, 6);
    glass.setTextColor(Theme::LCD_INK);
    glass.drawString(shown, 6, 5);

    glass.setTextSize(1);
    glass.drawString(credit, 6, 25);
    glass.drawString(status, 18, 39);
    if (playing)
    {
        glass.fillTriangle(6, 38, 6, 46, 12, 42, Theme::LCD_INK);
    }
    else
    {
        glass.fillRect(6, 38, 2, 8, Theme::LCD_INK);
        glass.fillRect(10, 38, 2, 8, Theme::LCD_INK);
    }
    glass.pushSprite(X, Y);
    glass.deleteSprite();
}
