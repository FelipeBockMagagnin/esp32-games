#ifndef LCD_PANEL_H
#define LCD_PANEL_H

#include <TFT_eSPI.h>

// The record player's display: the title, scrolling when it does not fit, the author and album,
// and a status line. Drawn into a sprite and pushed whole, so nothing flickers.
class LcdPanel
{
public:
    static const int16_t X = 6;
    static const int16_t Y = 6;
    static const int16_t W = 228;
    static const int16_t H = 52;

    void set(const char *title, const char *credit, const char *status, bool playing);
    // Pushes the panel when it changed or the title's scroll moved on
    void draw(TFT_eSPI &tft, unsigned long now);
    void invalidate() { dirty = true; }

private:
    char title[40] = "";
    char credit[64] = "";
    char status[40] = "";
    bool playing = false;
    bool dirty = true;
    int16_t scroll = 0;
    unsigned long nextStep = 0;
};

#endif // LCD_PANEL_H
