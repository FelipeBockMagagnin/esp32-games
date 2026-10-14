#ifndef UI_PRESS_FEEDBACK_H
#define UI_PRESS_FEEDBACK_H

#include "ListRow.h"

// What a press on a list row or tile did: the row flashes when the action went through, and
// the element shakes side to side when it was refused. The shake moves the element, so it
// must sit on the colour it erases with, with a few pixels clear on either side.
// Effects start at the time of the last update(), which the screen calls every loop.
class PressFeedback
{
public:
    void flash(ListRow &row);
    void shake(UI &element);
    void update(unsigned long now);
    bool isShaking() const { return shaking != nullptr; }

private:
    ListRow *flashing = nullptr;
    unsigned long flashStart = 0;
    UI *shaking = nullptr;
    int16_t homeX = 0;
    unsigned long shakeStart = 0;
    unsigned long now = 0;
};

#endif // UI_PRESS_FEEDBACK_H
