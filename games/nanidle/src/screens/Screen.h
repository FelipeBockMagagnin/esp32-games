#ifndef SCREEN_H
#define SCREEN_H

#include <Arduino.h>
#include <TFT_eSPI.h>
#include <vector>
#include "UI.h"
#include "../ui/Theme.h"
#include "../ui/Win.h"

enum class Key : uint8_t
{
    UP,
    DOWN,
    LEFT,
    RIGHT,
    CONFIRM
};

// A screen is a maximized window: its frame comes first, the taskbar shared by every screen
// last, and the body in between (Win::BODY_*). A dialog passes no title and draws its own.
class Screen
{
protected:
    std::vector<UI *> elements;
    Color bgColor;

    // Text rebuilds are gated to this interval; input handlers call forceRefresh() to skip it
    unsigned long refreshMs = 100;
    unsigned long lastRefresh = 0;

    bool refreshDue(unsigned long now)
    {
        if (lastRefresh != 0 && now - lastRefresh < refreshMs)
        {
            return false;
        }
        lastRefresh = now;
        return true;
    }
    void forceRefresh() { lastRefresh = 0; }

    // Call last in the subclass's constructor, so the taskbar draws over nothing it shares
    void addTaskbar();

public:
    explicit Screen(const char *title, Color bgColor = Theme::FACE);
    virtual ~Screen() = default;

    static Taskbar &taskbar();

    void addElement(UI *element);
    // For an element drawn on a field or panel rather than the window face
    void addElement(UI *element, Color surface);
    void markAllDirty();

    void updateHeader(unsigned long now) { taskbar().update(now); }
    void applyNotice(const char *text, unsigned long until) { taskbar().setNotice(text, until); }
    void setNavLabels(const char *back, const char *next) { taskbar().setNav(back, next); }

    virtual void onEnter(TFT_eSPI &tft);
    // Clears and redraws every element, keeping the screen's state (after a theme change)
    void repaint(TFT_eSPI &tft);
    virtual void onExit();
    virtual void update(unsigned long now);
    virtual void render(TFT_eSPI &tft);

    virtual void onConfirmPress() {}
    // Sent at a steady pace while a button stays held; false stops the repeat until the next press
    virtual bool onRepeat(Key key) { return false; }
    virtual void onUpPress() {}
    virtual void onDownPress() {}

    // Return true to consume Left or Right. Screens that do not override them fall
    // through to ScreenManager, which steps through the rotation.
    virtual bool onBackPress() { return false; }
    virtual bool onRightPress() { return false; }

    Color getBgColor() const { return bgColor; }

private:
    WinFrame frame;
    bool framed;
};

#endif // SCREEN_H
