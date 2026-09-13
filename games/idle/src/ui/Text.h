#ifndef UI_TEXT_H
#define UI_TEXT_H

#include "UI.h"

class Text : public UI
{
public:
    String text;
    uint16_t color;
    uint16_t bgColor;
    uint8_t size;
    uint8_t datum;
    bool transparentBg;

    Text()
        : UI(0, 0, 0, 0),
          text(""),
          color(0xFFFF),
          bgColor(0x0000),
          size(1),
          datum(TL_DATUM),
          transparentBg(true),
          prevW(0),
          prevH(0),
          prevText("")
    {
    }

    Text(int16_t x, int16_t y, const String &text, uint16_t color = 0xFFFF, uint8_t size = 1, uint8_t datum = TL_DATUM, bool transparentBg = true, uint16_t bgColor = 0x0000)
        : UI(x, y, 0, 0),
          text(text),
          color(color),
          bgColor(bgColor),
          size(size),
          datum(datum),
          transparentBg(transparentBg),
          prevW(0),
          prevH(0),
          prevText("")
    {
    }

    void setText(const String &newText)
    {
        if (text != newText)
        {
            text = newText;
            markDirty();
        }
    }

    void setColor(uint16_t newColor)
    {
        if (color != newColor)
        {
            color = newColor;
            markDirty();
        }
    }

    void setBgColor(uint16_t newBg, bool transparent = false)
    {
        if (bgColor != newBg || transparentBg != transparent)
        {
            bgColor = newBg;
            transparentBg = transparent;
            markDirty();
        }
    }

    void setSize(uint8_t newSize)
    {
        if (size != newSize)
        {
            size = newSize;
            markDirty();
        }
    }

    void setDatum(uint8_t newDatum)
    {
        if (datum != newDatum)
        {
            datum = newDatum;
            markDirty();
        }
    }

    void draw(TFT_eSPI &tft) override;

private:
    int16_t prevW;
    int16_t prevH;
    String prevText;
};

#endif // UI_TEXT_H
