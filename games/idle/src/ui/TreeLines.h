#ifndef UI_TREE_LINES_H
#define UI_TREE_LINES_H

#include "UI.h"

// The connector lines of an indented list, drawn only in the indent left of each row and the
// gap above it, so it never overlaps a row and must be registered after them. A spark can run
// down a row's line from its parent and into the row.
class TreeLines : public UI
{
public:
    static const uint8_t MAX_SLOTS = 9;

    // Slot i's row sits at (x + depth * indent, top + i * pitch), rowH tall
    TreeLines(int16_t x, int16_t top, int16_t pitch, int16_t rowH, int16_t indent, uint8_t count,
              uint16_t lineColor, uint16_t sparkColor);

    // `through` has bit l set when column l carries a line past this row to a later sibling
    // of an ancestor; `continues` when the row's own line goes on to a later sibling of its own
    void setSlot(uint8_t slot, uint8_t depth, uint8_t through, bool continues);

    // fromSlot is the parent's slot, or -1 when it is scrolled above the list
    void spark(int8_t fromSlot, uint8_t toSlot);
    // True once when the spark reaches its row, or is cut short by the list changing under it
    bool takeArrival();
    uint8_t getSparkTarget() const { return sparkTo; }

    void update(unsigned long now);
    void draw(TFT_eSPI &tft) override;

protected:
    void erase(TFT_eSPI &tft) override {}

private:
    struct Slot
    {
        uint8_t depth;
        uint8_t through;
        bool continues;
    };

    int16_t top;
    int16_t pitch;
    int16_t rowH;
    int16_t indent;
    uint8_t slotCount;
    uint16_t lineColor;
    uint16_t sparkColor;
    Slot slots[MAX_SLOTS];
    bool layoutChanged;

    bool sparking;
    bool arrived;
    uint8_t sparkTo;
    int16_t sparkX0; // The line's column
    int16_t sparkY0;
    int16_t sparkY1; // The elbow
    int16_t sparkX1; // Just short of the row
    unsigned long sparkStart;
    unsigned long sparkMs;
    int16_t sparkX;
    int16_t sparkY;
    int16_t drawnSparkX;
    int16_t drawnSparkY;
    bool sparkDrawn;

    unsigned long now;
    unsigned long lastFrame;

    int16_t columnX(uint8_t column) const { return x + column * indent + indent / 2 - 1; }
    int16_t rowY(uint8_t slot) const { return top + slot * pitch; }
    void drawLines(TFT_eSPI &tft);
};

#endif // UI_TREE_LINES_H
