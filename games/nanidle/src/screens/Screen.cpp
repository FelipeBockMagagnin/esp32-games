#include "Screen.h"

Screen::Screen(const char *title, Color bgColor)
    : bgColor(bgColor), frame(title != nullptr ? title : ""), framed(title != nullptr)
{
    if (framed)
    {
        addElement(&frame);
    }
}

Taskbar &Screen::taskbar()
{
    static Taskbar shared;
    return shared;
}

void Screen::addTaskbar()
{
    addElement(&taskbar());
}

void Screen::addElement(UI *element)
{
    if (element != nullptr)
    {
        element->eraseColor = bgColor;
        elements.push_back(element);
    }
}

void Screen::addElement(UI *element, Color surface)
{
    if (element != nullptr)
    {
        element->eraseColor = surface;
        elements.push_back(element);
    }
}

void Screen::markAllDirty()
{
    for (auto *elem : elements)
    {
        elem->markDirty();
    }
}

void Screen::onEnter(TFT_eSPI &tft)
{
    repaint(tft);
}

void Screen::repaint(TFT_eSPI &tft)
{
    // The frame paints the whole window, so only a dialog needs the screen cleared first
    if (!framed)
    {
        tft.fillScreen(bgColor);
    }
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
