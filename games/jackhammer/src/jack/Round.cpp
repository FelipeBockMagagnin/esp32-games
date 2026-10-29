#include "Round.h"
#include <string.h>

void Round::start(uint32_t seed)
{
    rng.reseed(seed);
    active = true;
    dead = false;
    facing = SIDE_UP;
    points = 0;
    earned = 0;
    banked = 0;
    interval = START_INTERVAL;
    seconds = 0;
    memset(enemies, 0, sizeof(enemies));
    markCount = 0;
    events = 0;
    spawnIn = FIRST_SPAWN_SECONDS;
    // The Unity coroutine made its first change as soon as it started
    harderIn = 0;
}

void Round::harder()
{
    interval -= interval >= FAST_UNTIL ? FAST_STEP : SLOW_STEP;
}

void Round::spawn()
{
    for (Enemy &e : enemies)
    {
        if (!e.alive)
        {
            e.alive = true;
            e.side = (uint8_t)rng.below(SIDE_COUNT);
            e.look = (uint8_t)rng.below(256);
            e.distance = SPAWN_DISTANCE;
            events |= EVENT_SPAWN;
            return;
        }
    }
}

void Round::mark(const Enemy &e)
{
    if (markCount < MAX_MARKS)
    {
        marks[markCount++] = {e.side, e.look, e.distance};
    }
}

void Round::turn(int8_t direction)
{
    if (!active || dead)
    {
        return;
    }
    facing = (uint8_t)((facing + SIDE_COUNT + direction) % SIDE_COUNT);
    collide();
}

void Round::collide()
{
    for (Enemy &e : enemies)
    {
        if (!e.alive || dead)
        {
            continue;
        }
        if (e.side == facing && e.distance <= HEAD_REACH)
        {
            e.alive = false;
            // The Unity multiplier was recomputed a frame after the point, so a kill pays
            // the multiplier from before it
            earned += getMultiplier();
            points++;
            events |= EVENT_KILL;
            mark(e);
        }
        else if (e.distance <= BODY_REACH)
        {
            e.alive = false;
            dead = true;
            events |= EVENT_DEAD;
            mark(e);
        }
    }
}

void Round::step()
{
    if (!active || dead)
    {
        return;
    }
    seconds += STEP_SECONDS;

    harderIn -= STEP_SECONDS;
    while (harderIn <= 0)
    {
        harder();
        harderIn += DIFFICULTY_SECONDS;
    }

    spawnIn -= STEP_SECONDS;
    if (spawnIn <= 0)
    {
        spawn();
        // WaitForSeconds on an interval at or below zero waited one frame
        spawnIn = interval > STEP_SECONDS ? interval : STEP_SECONDS;
    }

    for (Enemy &e : enemies)
    {
        if (e.alive)
        {
            e.distance -= ENEMY_SPEED * STEP_SECONDS;
        }
    }
    collide();
}

uint8_t Round::takeEvents()
{
    uint8_t taken = events;
    events = 0;
    return taken;
}
