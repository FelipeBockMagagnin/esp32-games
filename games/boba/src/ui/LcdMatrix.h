#ifndef UI_LCD_MATRIX_H
#define UI_LCD_MATRIX_H

#include <Arduino.h>
#include <TFT_eSPI.h>
#include "../art/Sprites.h"

// The toy's 32x16 dot matrix. Everything is drawn to a bitmap and present() sends only the dots
// that changed, which at the original's pace of a couple of frames a second costs next to nothing.
class LcdMatrix
{
public:
    static const int16_t COLS = 32;
    static const int16_t ROWS = 16;
    static const int16_t DOT = 7; // One pixel of it is the gap between dots
    static const int16_t X = 8;
    static const int16_t Y = 104;
    static const uint8_t CHAR_W = 4;

    void clear() { memset(rows, 0, sizeof(rows)); }
    void set(int16_t x, int16_t y, bool on = true);
    void fill(int16_t x, int16_t y, int16_t w, int16_t h, bool on = true);
    void sprite(const Sprite &art, int16_t x, int16_t y, bool mirror = false, Pose pose = Pose{});
    // Drawn with its feet on `bottom`, 16 being the matrix's floor
    void spriteOn(const Sprite &art, int16_t x, int16_t bottom, bool mirror = false, Pose pose = Pose{});
    void text(int16_t x, int16_t y, const char *s);
    void textCentered(int16_t y, const char *s);

    // Lights out: every dot flips, as the original's LCD goes dark
    void setInverted(bool on) { inverted = on; }

    // The next present() sends every dot, after the panel was cleared
    void invalidate() { stale = true; }
    void present(TFT_eSPI &tft);

#ifdef PET_TOOLS
    bool isOn(int16_t x, int16_t y) const { return rows[y] >> x & 1; }
#endif

private:
    uint32_t rows[ROWS] = {};
    uint32_t shown[ROWS] = {};
    bool inverted = false;
    bool stale = true;
};

#endif // UI_LCD_MATRIX_H
