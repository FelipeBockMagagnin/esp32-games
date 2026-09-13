#ifndef SCREEN_H
#define SCREEN_H

#include <Arduino.h>
#include <TFT_eSPI.h>
#include <vector>
#include "./ui/UI.h"

class Screen
{
protected:
    std::vector<UI *> elements;
    uint16_t bgColor;

public:
    Screen(uint16_t bgColor = 0x0000);
    virtual ~Screen() = default;

    void addElement(UI *element);
    void markAllDirty();

    virtual void onEnter(TFT_eSPI &tft);
    virtual void onExit();
    virtual void update(unsigned long now);
    virtual void render(TFT_eSPI &tft);

    uint16_t getBgColor() const { return bgColor; }
    void setBgColor(uint16_t color) { bgColor = color; }
};

#endif // SCREEN_H
