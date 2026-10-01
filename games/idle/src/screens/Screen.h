#ifndef SCREEN_H
#define SCREEN_H

#include <Arduino.h>
#include <TFT_eSPI.h>
#include <vector>
#include "../ui/UI.h"
#include "../ui/Header.h"

class Screen
{
protected:
    std::vector<UI *> elements;
    uint16_t bgColor;
    Header *headerElement;

public:
    Screen(uint16_t bgColor = 0x0000);
    virtual ~Screen() = default;

    void addElement(UI *element);
    void markAllDirty();

    // Screens register their header so the main loop can push a notification to whichever
    // screen is currently visible, without each one knowing about achievements
    void setHeader(Header *header);
    void applyNotice(const char *name);

    virtual void onEnter(TFT_eSPI &tft);
    virtual void onExit();
    virtual void update(unsigned long now);
    virtual void render(TFT_eSPI &tft);

    virtual void onConfirmPress() {}
    virtual void onSelectPress() {}

    // Return true to consume the back button. Screens that do not override it fall
    // through to ScreenManager, which steps back through the rotation.
    virtual bool onBackPress() { return false; }

    uint16_t getBgColor() const { return bgColor; }
    void setBgColor(uint16_t color);
};

#endif // SCREEN_H
