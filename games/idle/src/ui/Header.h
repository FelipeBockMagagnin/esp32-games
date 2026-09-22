#ifndef UI_HEADER_H
#define UI_HEADER_H

#include "UI.h"

// Screen header: centered title with optional navigation labels to the previous and next screens.
// An empty label hides that side, arrow included.
class Header : public UI
{
public:
    static const int16_t HEIGHT = 28;

    String title;
    String backLabel;
    String nextLabel;

    Header(const String &title = "", const String &backLabel = "", const String &nextLabel = "")
        : UI(0, 0, TFT_WIDTH, HEIGHT),
          title(title),
          backLabel(backLabel),
          nextLabel(nextLabel)
    {
    }

    void setTitle(const String &newTitle)
    {
        if (title != newTitle)
        {
            title = newTitle;
            markDirty();
        }
    }

    void setBackLabel(const String &label)
    {
        if (backLabel != label)
        {
            backLabel = label;
            markDirty();
        }
    }

    void setNextLabel(const String &label)
    {
        if (nextLabel != label)
        {
            nextLabel = label;
            markDirty();
        }
    }

    void draw(TFT_eSPI &tft) override;
};

#endif // UI_HEADER_H
