#ifndef UI_HEADER_H
#define UI_HEADER_H

#include "UI.h"

// Screen header: centered title with optional navigation labels to the previous and next screens.
// An empty label hides that side, arrow included.
//
// It doubles as the notification area: while a notice is set it takes over the whole bar,
// which is how an achievement or a newly unlocked screen gets shown on any screen without
// each one reserving space for it. The caption (size 1) names the kind of notice.
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
          nextLabel(nextLabel),
          notice(""),
          noticeCaption(nullptr),
          hasNotice(false)
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

    // nullptr or an empty name gives the bar back to the title and nav labels.
    // The caption is compared by pointer, so pass a string literal or other fixed storage.
    void setNotice(const char *caption, const char *name)
    {
        bool wanted = name != nullptr && name[0] != '\0';
        if (wanted == hasNotice && (!wanted || (notice == name && noticeCaption == caption)))
        {
            return;
        }
        hasNotice = wanted;
        notice = wanted ? name : "";
        noticeCaption = wanted ? caption : nullptr;
        markDirty();
    }

    void draw(TFT_eSPI &tft) override;

private:
    String notice;
    const char *noticeCaption;
    bool hasNotice;
};

#endif // UI_HEADER_H
