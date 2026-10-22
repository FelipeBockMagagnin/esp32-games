#include "Journal.h"
#include "GameState.h"
#include "CombatState.h"

static uint32_t satAdd32(uint32_t a, uint32_t b)
{
    return a > UINT32_MAX - b ? UINT32_MAX : a + b;
}

Journal::Journal() : data(), lastTick(0), started(false), seenBossKills(0)
{
}

void Journal::update(unsigned long now, const GameState &game, const CombatState &combat)
{
    if (!started)
    {
        started = true;
        lastTick = now;
    }
    // Whole seconds only, the rest carried to the next call
    unsigned long seconds = (now - lastTick) / 1000;
    if (seconds > 0)
    {
        data.runSeconds = satAdd32(data.runSeconds, seconds);
        lastTick += seconds * 1000;
    }

    if (combat.getBossKills() != seenBossKills)
    {
        data.bossKills = satAdd32(data.bossKills, combat.getBossKills() - seenBossKills);
        seenBossKills = combat.getBossKills();
    }
    if (game.getClickStreak() > data.bestStreak)
    {
        data.bestStreak = game.getClickStreak();
    }
}

void Journal::bankRun(const GameState &game, const CombatState &combat, uint32_t points)
{
    data.pastSeconds = satAdd32(data.pastSeconds, data.runSeconds);
    data.pastClicks = satAdd32(data.pastClicks, game.getTotalClicks());
    data.pastKills = satAdd32(data.pastKills, combat.getTotalKills());
    data.pastVeins = satAdd32(data.pastVeins, game.getVeinsCaught());
    for (uint8_t i = HISTORY - 1; i > 0; i--)
    {
        data.history[i] = data.history[i - 1];
    }
    data.history[0] = {data.runSeconds, points};
    if (data.runCount < HISTORY)
    {
        data.runCount++;
    }
    data.runSeconds = 0;
}

uint32_t Journal::getLifetimeClicks(const GameState &game) const
{
    return satAdd32(data.pastClicks, game.getTotalClicks());
}

uint32_t Journal::getLifetimeKills(const CombatState &combat) const
{
    return satAdd32(data.pastKills, combat.getTotalKills());
}

uint32_t Journal::getLifetimeVeins(const GameState &game) const
{
    return satAdd32(data.pastVeins, game.getVeinsCaught());
}

void Journal::load(const Snapshot &in)
{
    data = in;
    if (data.runCount > HISTORY)
    {
        data.runCount = HISTORY;
    }
}
