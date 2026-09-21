#include "Screen.h"

Screen::Screen(uint16_t bgColor)
    : bgColor(bgColor)
{
}

void Screen::addElement(UI *element)
{
    if (element != nullptr)
    {
        element->eraseColor = bgColor;
        elements.push_back(element);
    }
}

void Screen::setBgColor(uint16_t color)
{
    bgColor = color;
    for (auto *elem : elements)
    {
        elem->eraseColor = color;
    }
}

void Screen::markAllDirty()
{
    for (auto *elem : elements)
    {
        if (elem != nullptr)
        {
            elem->markDirty();
        }
    }
}

void Screen::onEnter(TFT_eSPI &tft)
{
    tft.fillScreen(bgColor);

    // The whole screen was just cleared, so nothing needs erasing before the first draw
    for (auto *elem : elements)
    {
        elem->resetDrawn();
    }

    render(tft);
}

void Screen::onExit()
{
}

void Screen::update(unsigned long now)
{
}

void Screen::render(TFT_eSPI &tft)
{
    for (auto *elem : elements)
    {
        elem->redraw(tft);
    }
}
