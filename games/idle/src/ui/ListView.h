#ifndef UI_LIST_VIEW_H
#define UI_LIST_VIEW_H

#include <Arduino.h>

// Selection and scroll arithmetic for a vertical list. Draws nothing: the screen owns
// a fixed array of ListRow and fills row i from item getFirstVisible() + i.
class ListView
{
public:
    static const uint8_t VISIBLE_ROWS = 9;

    ListView() : count(0), selectedIndex(0), scrollOffset(0) {}

    void setCount(uint16_t newCount)
    {
        count = newCount;
        if (selectedIndex >= count)
        {
            selectedIndex = count > 0 ? count - 1 : 0;
        }
        clampScroll();
    }

    // Cycles forward, wrapping back to the top past the last item
    void next()
    {
        if (count == 0)
        {
            return;
        }
        selectedIndex = (selectedIndex + 1) % count;
        clampScroll();
    }

    void setSelected(uint16_t index)
    {
        if (index < count)
        {
            selectedIndex = index;
            clampScroll();
        }
    }

    void reset()
    {
        selectedIndex = 0;
        scrollOffset = 0;
    }

    uint16_t getCount() const { return count; }
    uint16_t getSelected() const { return selectedIndex; }
    uint16_t getFirstVisible() const { return scrollOffset; }

    uint8_t getVisibleCount() const
    {
        uint16_t remaining = count - scrollOffset;
        return remaining < VISIBLE_ROWS ? (uint8_t)remaining : VISIBLE_ROWS;
    }

private:
    void clampScroll()
    {
        if (count <= VISIBLE_ROWS)
        {
            scrollOffset = 0;
            return;
        }

        if (selectedIndex < scrollOffset)
        {
            scrollOffset = selectedIndex;
        }
        else if (selectedIndex >= scrollOffset + VISIBLE_ROWS)
        {
            scrollOffset = selectedIndex - VISIBLE_ROWS + 1;
        }

        if (scrollOffset + VISIBLE_ROWS > count)
        {
            scrollOffset = count - VISIBLE_ROWS;
        }
    }

    uint16_t count;
    uint16_t selectedIndex;
    uint16_t scrollOffset;
};

#endif // UI_LIST_VIEW_H
