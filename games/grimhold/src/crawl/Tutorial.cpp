#include "Tutorial.h"
#include <math.h>

static const float WALK_DISTANCE = 1.0f;
static const float TURN_ANGLE = 1.2f;

static const char *const HINTS[Tutorial::STEP_COUNT] = {
    "Tutorial 1/5: Up and Down walk",
    "Tutorial 2/5: Left and Right turn",
    "Tutorial 3/5: tap Confirm to swing the sword",
    "Tutorial 4/5: hold Confirm, let go to cast",
    "Tutorial 5/5: Up + Down together pause",
};

void Tutorial::start(const Run &run)
{
    step = WALK;
    startX = run.getX();
    startY = run.getY();
    lastAngle = run.getAngle();
    turned = 0;
    pauseShownMs = 0;
}

const char *Tutorial::hint() const
{
    return step < DONE ? HINTS[step] : "";
}

bool Tutorial::inStartRoom(const Run &run) const
{
    // A tile of slack, so standing in the doorway does not end it
    const Room &r = run.getMap().rooms[0];
    return run.getX() >= r.x - 1 && run.getX() < r.x + r.w + 1 && run.getY() >= r.y - 1 &&
           run.getY() < r.y + r.h + 1;
}

bool Tutorial::update(const Run &run, uint32_t events, uint16_t dtMs)
{
    if (step == DONE)
    {
        return false;
    }
    float delta = run.getAngle() - lastAngle;
    lastAngle = run.getAngle();
    if (delta > 3.14159265f)
    {
        delta -= 6.2831853f;
    }
    else if (delta < -3.14159265f)
    {
        delta += 6.2831853f;
    }
    turned += fabsf(delta);

    if (run.getFloor() != 1 || !inStartRoom(run))
    {
        step = DONE;
        return true;
    }
    Step before = step;
    switch (step)
    {
    case WALK:
        if (hypotf(run.getX() - startX, run.getY() - startY) >= WALK_DISTANCE)
        {
            step = TURN;
            turned = 0;
        }
        break;
    case TURN:
        if (turned >= TURN_ANGLE)
        {
            step = SWORD;
        }
        break;
    case SWORD:
        if (events & EV_SWING)
        {
            step = SPELL;
        }
        break;
    case SPELL:
        if (events & (EV_CAST | EV_NO_MANA))
        {
            step = PAUSE;
        }
        break;
    case PAUSE:
        pauseShownMs += dtMs;
        if (pauseShownMs >= PAUSE_HINT_MS)
        {
            step = DONE;
        }
        break;
    default:
        break;
    }
    return step != before;
}
