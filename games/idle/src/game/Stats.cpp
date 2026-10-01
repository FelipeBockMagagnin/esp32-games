#include "Stats.h"
#include "GameState.h"
#include "Inventory.h"

Stats resolvePlayerStats(const GameState &game, const Inventory &inv)
{
    Stats equipped = inv.getEquippedStats();

    Stats total;
    total.attack = (BASE_ATTACK + equipped.attack) * (100 + game.getAttackBonusPercent()) / 100;
    total.defense = (BASE_DEFENSE + equipped.defense) * (100 + game.getDefenseBonusPercent()) / 100;
    total.maxHp = (BASE_MAX_HP + equipped.maxHp) * (100 + game.getMaxHpBonusPercent()) / 100;
    return total;
}
