#ifndef SCREEN_H
#define SCREEN_H

#include <Arduino.h>
#include <TFT_eSPI.h>
#include <vector>
#include "../ui/UI.h"
#include "../ui/Header.h"
#include "../ui/Theme.h"

class Screen
{
protected:
    std::vector<UI *> elements;
    uint16_t bgColor;
    Header *headerElement;

public:
    Screen(uint16_t bgColor = Theme::BG);
    virtual ~Screen() = default;

    void addElement(UI *element);
    // For an element drawn on a filled card or panel rather than the screen background:
    // it erases with the card's colour, so updating it does not punch a hole in the card
    void addElement(UI *element, uint16_t surface);
    void markAllDirty();

    // Screens register their header so the main loop can push a notification to whichever
    // screen is currently visible, without each one knowing about achievements, and so
    // ScreenManager can label it with whichever neighbours are unlocked
    void setHeader(Header *header);
    void applyNotice(const char *caption, const char *name, uint16_t accent);
    void setNavLabels(const char *back, const char *next);

    virtual void onEnter(TFT_eSPI &tft);
    virtual void onExit();
    virtual void update(unsigned long now);
    virtual void render(TFT_eSPI &tft);

    virtual void onConfirmPress() {}
    virtual void onUpPress() {}
    virtual void onDownPress() {}
    virtual void onSelectPress() { onDownPress(); }

    // Return true to consume the back button. Screens that do not override it fall
    // through to ScreenManager, which steps back through the rotation.
    virtual bool onBackPress() { return false; }

    uint16_t getBgColor() const { return bgColor; }
    void setBgColor(uint16_t color);
};

#endif // SCREEN_H
