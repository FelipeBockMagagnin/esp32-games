#ifndef SCREEN_H
#define SCREEN_H

#include <Arduino.h>
#include <TFT_eSPI.h>
#include <vector>
#include "UI.h"
#include "../ui/Header.h"
#include "../ui/Theme.h"

class Screen
{
protected:
    std::vector<UI *> elements;
    Color bgColor;
    Header *headerElement;

    // Text rebuilds are gated to this interval while animations advance every frame;
    // input handlers call forceRefresh() to skip the wait
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

public:
    Screen(Color bgColor = Theme::BG);
    virtual ~Screen() = default;

    void addElement(UI *element);
    // For an element drawn on a filled card or panel rather than the screen background:
    // it erases with the card's colour, so updating it does not punch a hole in the card
    void addElement(UI *element, Color surface);
    void markAllDirty();

    // Screens register their header so the main loop can push a notification to whichever
    // screen is currently visible, without each one knowing about achievements, and so
    // ScreenManager can label it with whichever neighbours are unlocked
    void setHeader(Header *header);
    void updateHeader(unsigned long now)
    {
        if (headerElement != nullptr)
        {
            headerElement->update(now);
        }
    }
    void applyNotice(const char *caption, const char *name, Color accent);
    void setNavLabels(const char *back, const char *next);

    virtual void onEnter(TFT_eSPI &tft);
    // Clears and redraws every element, keeping the screen's state (after a theme change)
    void repaint(TFT_eSPI &tft);
    virtual void onExit();
    virtual void update(unsigned long now);
    virtual void render(TFT_eSPI &tft);

    virtual void onConfirmPress() {}
    // Sent while Confirm stays held, faster and faster; false stops the repeat until the next press
    virtual bool onConfirmRepeat() { return false; }
    virtual void onUpPress() {}
    virtual void onDownPress() {}
    virtual void onSelectPress() { onDownPress(); }

    // Return true to consume the back button. Screens that do not override it fall
    // through to ScreenManager, which steps back through the rotation.
    virtual bool onBackPress() { return false; }

    Color getBgColor() const { return bgColor; }
    void setBgColor(Color color);
};

#endif // SCREEN_H
