#ifndef UI_LIST_VIEW_H
#define UI_LIST_VIEW_H

#include <Arduino.h>

// Selection and scroll arithmetic for a vertical list. Draws nothing: the screen owns
// a fixed array of ListRow and fills row i from item getFirstVisible() + i.
class ListView
{
public:
    // Default and maximum rows on screen; size row arrays with this. A screen that
    // shows fewer (to make room for a panel) passes its count to the constructor.
    static const uint8_t VISIBLE_ROWS = 9;

    ListView(uint8_t visibleRows = VISIBLE_ROWS)
        : count(0), selectedIndex(0), scrollOffset(0),
          visibleRows(visibleRows < VISIBLE_ROWS ? visibleRows : VISIBLE_ROWS) {}

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

    // Cycles backward, wrapping to the bottom from the top item
    void previous()
    {
        if (count == 0)
        {
            return;
        }
        selectedIndex = (selectedIndex == 0) ? (count - 1) : (selectedIndex - 1);
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
        return remaining < visibleRows ? (uint8_t)remaining : visibleRows;
    }

private:
    void clampScroll()
    {
        if (count <= visibleRows)
        {
            scrollOffset = 0;
            return;
        }

        if (selectedIndex < scrollOffset)
        {
            scrollOffset = selectedIndex;
        }
        else if (selectedIndex >= scrollOffset + visibleRows)
        {
            scrollOffset = selectedIndex - visibleRows + 1;
        }

        if (scrollOffset + visibleRows > count)
        {
            scrollOffset = count - visibleRows;
        }
    }

    uint16_t count;
    uint16_t selectedIndex;
    uint16_t scrollOffset;
    uint8_t visibleRows;
};

#endif // UI_LIST_VIEW_H
