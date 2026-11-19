#ifndef TURNTABLE_H
#define TURNTABLE_H

#include <TFT_eSPI.h>
#include "../music/Tracks.h"

// The record player: a wooden plinth, the platter with the record on it and the tone arm.
// The record is drawn pixel by pixel each frame from its radius, so the grooves cost nothing to
// spin; only the label is turned, sampled from a small picture of it. The sheen on the vinyl stays
// put, as a lamp's would. Frames go out in bands of rows through one small buffer.
class Turntable
{
public:
    // The plinth on screen
    static const int16_t PLINTH_X = 4;
    static const int16_t PLINTH_Y = 64;
    static const int16_t PLINTH_W = 232;
    static const int16_t PLINTH_H = 192;

    void begin();

    // The record on the platter: its label, and a lighter groove between each two tracks
    void setAlbum(uint8_t album);
    // Where the needle should be: 0 the first groove, 1 the last; negative lifts it to its rest
    void setNeedle(float progress);
    void setSpinning(bool on) { spinning = on; }

    // Advances the platter and the arm; false when nothing moved since the last draw
    bool update(unsigned long now);
    // The whole plinth, for a fresh screen
    void drawPlinth(TFT_eSPI &tft);
    void render(TFT_eSPI &tft);

private:
    static const int16_t X = 12; // The region drawn each frame
    static const int16_t Y = 70;
    static const int16_t W = 218;
    static const int16_t H = 182;
    static const int16_t BAND = 26;
    static const int16_t CX = 104; // The record's centre and radius, on screen
    static const int16_t CY = 160;
    static const int16_t R = 86;
    static const int16_t RIM = 3; // The platter's edge beyond the record
    static const int16_t LABEL_R = 30;
    static const int16_t LABEL_D = LABEL_R * 2 + 1;
    static const int16_t PIVOT_X = 206;
    static const int16_t PIVOT_Y = 92;
    static const int16_t ARM_LEN = 112;
    static const int16_t GROOVE_OUT = 82;
    static const int16_t GROOVE_IN = 37;

    uint16_t band[W * BAND];
    uint16_t label[LABEL_D * LABEL_D];
    uint8_t root[(R + RIM) * (R + RIM) + 1]; // sqrt of r^2, for the radius of each pixel
    uint16_t rings[3][R + RIM + 1];          // Colour by radius, at each strength of sheen
    uint16_t wood[2];
    uint16_t metal[3];
    uint8_t album = 255;
    uint8_t theme = 255;

    bool spinning = false;
    float speed = 0;   // Turns a second
    float turn = 0;    // 0..1
    float needle = -1; // As setNeedle()
    float armAngle;    // Radians, on screen
    float armTarget;
    float drawnTurn = -1;
    float drawnArm = -1;
    unsigned long last = 0;

    void buildColors();
    void buildLabel();
    float angleFor(float progress) const;
    uint16_t woodAt(int16_t sx, int16_t sy) const;

    void put(int16_t sx, int16_t sy, int16_t top, uint16_t c);
    void disc(float cx, float cy, float r, int16_t top, uint16_t c);
    void rod(float x0, float y0, float x1, float y1, float r, int16_t top, uint16_t c);
    void drawArm(int16_t top);
};

#endif // TURNTABLE_H
