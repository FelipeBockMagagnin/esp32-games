#include "Screen.h"

Screen::Screen(Color bgColor)
    : bgColor(bgColor), headerElement(nullptr)
{
}

void Screen::setHeader(Header *header)
{
    headerElement = header;
}

void Screen::applyNotice(const char *caption, const char *name, Color accent)
{
    if (headerElement != nullptr)
    {
        headerElement->setNotice(caption, name, accent);
    }
}

void Screen::setNavLabels(const char *back, const char *next)
{
    if (headerElement != nullptr)
    {
        headerElement->setBackLabel(back);
        headerElement->setNextLabel(next);
    }
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

void Screen::setBgColor(Color color)
{
    // Elements on a card keep the card's colour
    for (auto *elem : elements)
    {
        if (elem->eraseColor == bgColor)
        {
            elem->eraseColor = color;
        }
    }
    bgColor = color;
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
    repaint(tft);
}

void Screen::repaint(TFT_eSPI &tft)
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
