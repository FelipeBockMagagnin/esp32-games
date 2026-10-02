#ifndef UI_LIST_ROW_H
#define UI_LIST_ROW_H

#include "UI.h"

// One row of a vertical list: optional icon, a title with a subtitle under it, and
// two right-aligned value fields. Screens keep a fixed array of these and rewrite
// their contents as the list scrolls, so no element is ever added or removed.
class ListRow : public UI
{
public:
    enum State : uint8_t
    {
        NORMAL,
        SELECTED, // Yellow frame; the row the buttons act on
        DIMMED,   // Gray; locked or unavailable
        OWNED     // Green frame; already taken or currently active
    };

    static const int16_t DEFAULT_W = 230;
    static const int16_t DEFAULT_H = 26;
    static const uint16_t DEFAULT_FRAME_COLOR = 0xFFFF;

    ListRow(int16_t x = 0, int16_t y = 0, int16_t w = DEFAULT_W, int16_t h = DEFAULT_H)
        : UI(x, y, w, h),
          title(""),
          subtitle(""),
          value(""),
          valueSub(""),
          state(NORMAL),
          valueColor(0xFFFF),
          showCoin(false),
          iconBitmap(nullptr),
          iconPixels(nullptr),
          iconW(0),
          iconH(0),
          iconColor(0xFFFF),
          frameColor(DEFAULT_FRAME_COLOR),
          selectedFill(0x0000),
          hasSelectedFill(false),
          titleColorOverride(0xFFFF),
          hasTitleColor(false)
    {
    }

    void setTitle(const String &v) { assign(title, v); }
    void setSubtitle(const String &v) { assign(subtitle, v); }
    void setValue(const String &v) { assign(value, v); }
    void setValueSub(const String &v) { assign(valueSub, v); }

    void setState(State v)
    {
        if (state != v)
        {
            state = v;
            markDirty();
        }
    }

    void setValueColor(uint16_t v)
    {
        if (valueColor != v)
        {
            valueColor = v;
            markDirty();
        }
    }

    // Draws a small gold dot left of the value, matching the coin marker used elsewhere
    void setShowCoin(bool v)
    {
        if (showCoin != v)
        {
            showCoin = v;
            markDirty();
        }
    }

    // Title color for every state but DIMMED, which stays gray; overrides OWNED's green title
    void setTitleColor(uint16_t v)
    {
        if (!hasTitleColor || titleColorOverride != v)
        {
            titleColorOverride = v;
            hasTitleColor = true;
            markDirty();
        }
    }

    // Frame of a NORMAL row; the other states keep their own colors
    void setFrameColor(uint16_t v)
    {
        if (frameColor != v)
        {
            frameColor = v;
            markDirty();
        }
    }

    // Fills a SELECTED row's background, so the selection reads as a highlight bar
    // rather than only a frame. Text and icons are drawn over it.
    void setSelectedFill(uint16_t v)
    {
        if (!hasSelectedFill || selectedFill != v)
        {
            selectedFill = v;
            hasSelectedFill = true;
            markDirty();
        }
    }

    void setIconBitmap(const unsigned char *bitmap, int16_t bw, int16_t bh, uint16_t color = 0xFFFF);
    void setIconPixels(const uint16_t *pixels, int16_t pw, int16_t ph);
    void clearIcon();

    void draw(TFT_eSPI &tft) override;

private:
    void assign(String &field, const String &v)
    {
        if (field != v)
        {
            field = v;
            markDirty();
        }
    }

    String title;
    String subtitle;
    String value;
    String valueSub;

    State state;
    uint16_t valueColor;
    bool showCoin;

    const unsigned char *iconBitmap;
    const uint16_t *iconPixels;
    int16_t iconW;
    int16_t iconH;
    uint16_t iconColor;

    uint16_t frameColor;
    uint16_t selectedFill;
    bool hasSelectedFill;
    uint16_t titleColorOverride;
    bool hasTitleColor;
};

#endif // UI_LIST_ROW_H
