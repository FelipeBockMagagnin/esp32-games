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
    uint16_t eraseColor; // Color used to clear the element when it is hidden or moved

    UI() : UI(0, 0, 0, 0) {}
    UI(int16_t x, int16_t y, int16_t w = 0, int16_t h = 0)
        : x(x), y(y), w(w), h(h), visible(true), dirty(true), eraseColor(0x0000),
          drawn(false), drawnX(0), drawnY(0), drawnW(0), drawnH(0) {}

    virtual ~UI() = default;

    virtual void init() {}
    virtual void draw(TFT_eSPI &tft) = 0;

    void redraw(TFT_eSPI &tft)
    {
        if (!dirty)
        {
            return;
        }

        // Clear the previously drawn area when hidden, moved or resized
        if (drawn && (!visible || x != drawnX || y != drawnY || w != drawnW || h != drawnH))
        {
            erase(tft);
            drawn = false;
        }

        if (visible)
        {
            draw(tft);
            drawn = true;
            drawnX = x;
            drawnY = y;
            drawnW = w;
            drawnH = h;
        }

        dirty = false;
    }

    void resetDrawn()
    {
        drawn = false;
        markDirty();
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

protected:
    // Clears the area of the last draw; elements whose drawn area differs from x/y/w/h override it
    virtual void erase(TFT_eSPI &tft)
    {
        if (drawnW > 0 && drawnH > 0)
        {
            tft.fillRect(drawnX, drawnY, drawnW, drawnH, eraseColor);
        }
    }

    bool drawn;
    int16_t drawnX;
    int16_t drawnY;
    int16_t drawnW;
    int16_t drawnH;
};

#endif // UI_BASE_H
