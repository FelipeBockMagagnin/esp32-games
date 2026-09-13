#ifndef UI_BASE_H
#define UI_BASE_H

#include <Arduino.h>
#include <TFT_eSPI.h>

class UI
{
public:
    int16_t x;
    int16_t y;
    int16_t w;
    int16_t h;
    bool visible;
    bool dirty;

    UI() : x(0), y(0), w(0), h(0), visible(true), dirty(true) {}
    UI(int16_t x, int16_t y, int16_t w = 0, int16_t h = 0)
        : x(x), y(y), w(w), h(h), visible(true), dirty(true) {}

    virtual ~UI() = default;

    virtual void init() {}
    virtual void draw(TFT_eSPI &tft) = 0;
    virtual void redraw(TFT_eSPI &tft)
    {
        if (visible && dirty)
        {
            draw(tft);
            dirty = false;
        }
    }

    void markDirty()
    {
        dirty = true;
    }

    bool isDirty() const
    {
        return dirty;
    }

    void setPosition(int16_t newX, int16_t newY)
    {
        if (x != newX || y != newY)
        {
            x = newX;
            y = newY;
            markDirty();
        }
    }

    void setSize(int16_t newW, int16_t newH)
    {
        if (w != newW || h != newH)
        {
            w = newW;
            h = newH;
            markDirty();
        }
    }

    void setVisible(bool v)
    {
        if (visible != v)
        {
            visible = v;
            markDirty();
        }
    }

    bool isVisible() const
    {
        return visible;
    }

    bool contains(int16_t px, int16_t py) const
    {
        return px >= x && px < (x + w) && py >= y && py < (y + h);
    }
};

#endif // UI_BASE_H
