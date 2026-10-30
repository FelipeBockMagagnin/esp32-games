#ifndef UI_PANEL_H
#define UI_PANEL_H

#include <Arduino.h>
#include <TFT_eSPI.h>
#include "LcdMatrix.h"

// The toy's face around the matrix: the shell's edge and the two rows of LCD icons
class Panel
{
public:
    static const int16_t ICON_X = 9;
    static const int16_t ICON_PITCH = 58;
    static const int16_t ICON_TOP_Y = 36;
    static const int16_t ICON_BOTTOM_Y = 236;
    static const int16_t ICON_DOT = 3;
    static const int8_t NONE = -1;

    explicit Panel(LcdMatrix &lcd) : lcd(lcd) {}

    // Everything from scratch, after a theme change or a modal
    void paint(TFT_eSPI &tft);
    // Lights the selected icon and the attention one, redrawing only what changed
    void setIcons(TFT_eSPI &tft, int8_t selected, bool calling);

private:
    LcdMatrix &lcd;
    int8_t shownSelected = NONE;
    bool shownCalling = false;

    void drawIcon(TFT_eSPI &tft, uint8_t icon, bool on);
};

#endif // UI_PANEL_H
