#ifndef CRAWL_TUTORIAL_H
#define CRAWL_TUTORIAL_H

#include "Run.h"

// The controls, taught one at a time in the first floor's starting room: each hint stays until the
// player does what it says. Walking out of the room ends it too, done or not.
class Tutorial
{
public:
    enum Step : uint8_t
    {
        WALK,
        TURN,
        SWORD,
        SPELL,
        PAUSE,
        DONE
    };
    static const uint8_t STEP_COUNT = DONE;
    static const uint16_t PAUSE_HINT_MS = 4000;

    void start(const Run &run);
    // True when the step changed
    bool update(const Run &run, uint32_t events, uint16_t dtMs);

    Step getStep() const { return step; }
    bool isDone() const { return step == DONE; }
    const char *hint() const;

private:
    Step step = DONE;
    float startX = 0, startY = 0;
    float lastAngle = 0;
    float turned = 0;
    uint16_t pauseShownMs = 0;

    bool inStartRoom(const Run &run) const;
};

#endif // CRAWL_TUTORIAL_H
