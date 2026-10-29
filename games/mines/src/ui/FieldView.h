#ifndef UI_FIELD_VIEW_H
#define UI_FIELD_VIEW_H

#include <TFT_eSPI.h>
#include "../mines/Field.h"

// Draws a Field on the panel and plays its animations a frame per call, so the screens keep
// the pace: the reveal of what an open just uncovered, ring by ring, each tile sinking open
// over a few frames; the sweep across the diagonals when a field is cleared; the field
// covering over again.
class FieldView
{
public:
    FieldView(TFT_eSPI &tft, const Field &field) : tft(tft), field(field) {}

    void place(int16_t x, int16_t y, int16_t tile);
    int16_t tile() const { return s; }
    int16_t width() const { return field.cols() * s; }
    int16_t height() const { return field.rows() * s; }
    void cellOrigin(Cell c, int16_t &px, int16_t &py) const;
    uint8_t parity(Cell c) const { return (uint8_t)(c.x * 7 + c.y * 5); }

    // The cell as the field has it now
    void draw(Cell c);
    void drawAll();

    // Takes over the cells the field's last open listed, which stay drawn covered until
    // their wave comes up
    void beginReveal();
    bool revealing() const { return revealOn; }
    // The wave that just started sinking, for its note, or -1
    int16_t revealStep();
    void finishReveal();

    // One frame of the sweep at diagonal d (x + y): flags rise over the mines and the open
    // tiles glint. Done once d passes diagonals() + TileArt::FLAG_FRAMES.
    void sweep(uint8_t d);
    // Draws diagonal d covered
    void cover(uint8_t d);
    uint8_t diagonals() const { return field.cols() + field.rows() - 1; }

private:
    TFT_eSPI &tft;
    const Field &field;
    int16_t ox = 0, oy = 0, s = 16;

    bool revealOn = false;
    uint16_t revealStart = 0; // The first listed cell not yet drawn open
    int16_t revealT = 0;
};

#endif // UI_FIELD_VIEW_H
