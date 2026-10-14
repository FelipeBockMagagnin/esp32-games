#ifndef UI_SCREEN_WIPE_H
#define UI_SCREEN_WIPE_H

#include "UI.h"

// Light spreading up and down from the middle of the screen, from `color` at its heart to
// the text colour at its edges, ending on a full flash. Paints only the rows it gained each
// frame. Register it last, so it covers everything.
class ScreenWipe : public UI
{
public:
    explicit ScreenWipe(uint16_t color);

    void start(unsigned long now);
    void stop();
    bool isDone() const { return done; }

    void update(unsigned long now);
    void draw(TFT_eSPI &tft) override;

protected:
    void erase(TFT_eSPI &tft) override {}

private:
    uint16_t color;
    unsigned long startTime;
    int16_t reach;      // Rows from the middle the light should cover
    int16_t drawnReach; // Rows it covers on the panel
    bool done;
    unsigned long lastFrame;
};

#endif // UI_SCREEN_WIPE_H
