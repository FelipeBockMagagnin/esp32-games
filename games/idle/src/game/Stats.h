#ifndef GAME_STATS_H
#define GAME_STATS_H

#include <Arduino.h>

struct Stats
{
    uint32_t attack;
    uint32_t defense;
    uint32_t maxHp;
};

// What the player is worth with nothing equipped, so the first zone is fightable bare
static const uint32_t BASE_ATTACK = 5;
static const uint32_t BASE_DEFENSE = 1;
static const uint32_t BASE_MAX_HP = 50;

class GameState;
class Inventory;

// Single source of truth for the player's combat stats: equipment plus upgrade bonuses
Stats resolvePlayerStats(const GameState &game, const Inventory &inv);

#endif // GAME_STATS_H
