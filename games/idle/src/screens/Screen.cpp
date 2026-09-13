#include "Screen.h"

Screen::Screen(uint16_t bgColor)
    : bgColor(bgColor)
{
}

void Screen::addElement(UI *element)
{
    if (element != nullptr)
    {
        elements.push_back(element);
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
    markAllDirty();
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
        if (elem != nullptr && elem->isDirty())
        {
            elem->redraw(tft);
        }
    }
}
