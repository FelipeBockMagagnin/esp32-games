#ifndef UI_HEADER_H
#define UI_HEADER_H

#include "UI.h"
#include "Theme.h"

// Screen header: centered title with optional navigation labels to the previous and next screens.
// An empty label hides that side, arrow included.
//
// It doubles as the notification area: while a notice is set it takes over the whole bar,
// which is how an achievement or a newly unlocked screen gets shown on any screen without
// each one reserving space for it. The caption (size 1) names the kind of notice, and the
// accent color tints it and the bar's bottom rule so each kind is recognisable at a glance.
//
// A new notice flashes the bar in its accent and fades back, which needs update() every loop.
//
// The bar is painted in its own band color rather than the screen background, so it reads
// as a title bar on every screen.
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
          noticeAccent(Theme::TEXT),
          hasNotice(false),
          flashLevel(0),
          lastFlashStep(0)
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
    void setNotice(const char *caption, const char *name, uint16_t accent)
    {
        bool wanted = name != nullptr && name[0] != '\0';
        if (wanted == hasNotice &&
            (!wanted || (notice == name && noticeCaption == caption && noticeAccent == accent)))
        {
            return;
        }
        bool fresh = wanted && notice != name;
        hasNotice = wanted;
        flashLevel = fresh ? FLASH_STEPS : 0;
        notice = wanted ? name : "";
        noticeCaption = wanted ? caption : nullptr;
        noticeAccent = accent;
        markDirty();
    }

    void update(unsigned long now);

    void draw(TFT_eSPI &tft) override;

private:
    static const uint8_t FLASH_STEPS = 6;

    String notice;
    const char *noticeCaption;
    uint16_t noticeAccent;
    bool hasNotice;
    uint8_t flashLevel;
    unsigned long lastFlashStep;
};

#endif // UI_HEADER_H
