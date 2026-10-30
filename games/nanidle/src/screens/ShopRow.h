#ifndef SHOP_ROW_H
#define SHOP_ROW_H

#include "Screen.h"
#include "Text.h"
#include "../ui/Labels.h"

// A row of a shop screen: the name and what is owned on top, what it does under it, and the
// price on the right, which turns red while it cannot be paid
struct ShopRow
{
    FocusFrame frame;
    Text name;
    Text owned;
    Text detail;
    Text price;

    void place(Screen &screen, int16_t y, int16_t h)
    {
        frame = FocusFrame(5, y, 230, h - 2);
        name = label(10, y + 5);
        owned = label(230, y + 5, TR_DATUM, Theme::TEXT_2);
        detail = label(10, y + 18, TL_DATUM, Theme::TEXT_2);
        price = label(230, y + 18, TR_DATUM);
        screen.addElement(&name);
        screen.addElement(&owned);
        screen.addElement(&detail);
        screen.addElement(&price);
        screen.addElement(&frame);
    }

    void setPrice(const char *text, bool affordable)
    {
        price.setText(text);
        price.setColor(affordable ? Theme::GOOD : Theme::BAD);
    }
};

#endif // SHOP_ROW_H
