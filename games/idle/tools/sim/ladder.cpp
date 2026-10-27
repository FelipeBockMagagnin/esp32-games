// Fights every zone with the real CombatState at stages of gear, played actively (strike
// on cooldown, guard before each hit) and idle, and checks each stage against the
// difficulty ladder in CombatConfig.h. Build and run with tools/sim/ladder.sh.
// Usage: ladder [upgrade price factor, default 4] [minutes per fight, default 60] [zone]
#include <Arduino.h>
#include <cstdlib>
#include <random>

#include "../../src/game/GameState.h"
#include "../../src/game/Inventory.h"
#include "../../src/game/CombatState.h"
#include "../../src/game/Achievements.h"
#include "../../src/game/Ascension.h"
#include "../../src/game/Stats.h"

static unsigned long simNow = 0;
static std::mt19937 rng(1);
unsigned long millis() { return simNow; }
long random(long max) { return max <= 0 ? 0 : (long)(rng() % (unsigned long)max); }
long random(long min, long max) { return min + random(max - min); }

static bool zoneDrops(uint8_t zone, uint8_t item)
{
    return enemiesDropItem(ZONES[zone].enemies, ZONES[zone].enemyCount, item);
}

struct Stage
{
    const char *name;
    uint8_t earlierLevel; // The gear of the zones before, at this level
    uint8_t ownLevel;     // The zone's own set at this level, 0 for none
    // Kills an hour and deaths a kill the stage should land in; a negative bound is not
    // checked. The earlier gear at level 1 cannot win here even striking; a few levels up,
    // it wins only actively, at a death every few kills. The zone's own set makes active
    // play easy; a few of its levels let the auto attack farm it while dying often, more
    // levels make the deaths rare, and then take them away.
    double activeKillsMin, activeKillsMax, activeDeathsMin, activeDeathsMax;
    double idleKillsMin, idleKillsMax, idleDeathsMin, idleDeathsMax;
};

static const Stage STAGES[] = {
    //  name               earlier own  active kills  deaths/kill   idle kills    deaths/kill
    {"arrive, Lv 1 kit",  1,      0,   -1,   5,      -1,   -1,      -1,   -1,     -1,    -1},
    {"arrive, Lv 5 kit",  5,      0,   80,   -1,     0.3,  0.9,     -1,   0,      -1,    -1},
    {"own set Lv 1",      5,      1,   -1,   -1,     -1,   0.25,    -1,   -1,     -1,    -1},
    {"own set Lv 5",      5,      5,   -1,   -1,     -1,   -1,      15,   -1,     0.3,   2.0},
    {"own set Lv 10",     5,      10,  -1,   -1,     -1,   -1,      30,   -1,     0.03,  0.35},
    {"own set Lv 25",     5,      25,  -1,   -1,     -1,   -1,      60,   -1,     -1,    0.01},
};

struct Outcome
{
    double kills;  // An hour
    double deaths; // A kill, or the deaths themselves when nothing died
    Stats stats;
};

static Outcome fight(uint8_t zone, const Stage &stage, double factor, unsigned long minutes, bool active)
{
    Achievements achievements;
    Inventory inventory;
    Ascension ascension;
    GameState game{inventory, achievements, ascension};
    CombatState combat{game, inventory};

    // The zone's map and every combat upgrade priced within `factor` of it
    uint64_t mapCost = 0;
    for (uint8_t i = 0; i < GOLD_UPGRADE_COUNT; i++)
    {
        if (GOLD_UPGRADES[i].target == UpgradeTarget::UNLOCK_ZONE && GOLD_UPGRADES[i].bonusPercent == zone)
        {
            mapCost = GOLD_UPGRADES[i].cost;
        }
    }
    GameState::Snapshot snap = {};
    for (uint8_t i = 0; i < GOLD_UPGRADE_COUNT; i++)
    {
        UpgradeTarget t = GOLD_UPGRADES[i].target;
        bool combatUpgrade = t == UpgradeTarget::ATTACK || t == UpgradeTarget::DEFENSE || t == UpgradeTarget::MAX_HP;
        snap.goldUpgradesBought[i] = (combatUpgrade && GOLD_UPGRADES[i].cost <= mapCost * factor) ||
                                     (t == UpgradeTarget::UNLOCK_ZONE && GOLD_UPGRADES[i].bonusPercent <= zone);
    }
    game.load(snap);

    for (uint8_t slot = 0; slot < EQUIP_SLOT_COUNT; slot++)
    {
        int best = -1;
        uint8_t bestLevel = 1;
        double bestScore = -1;
        for (uint8_t i = 0; i < ITEM_COUNT; i++)
        {
            const ItemDef &item = ITEMS[i];
            if ((uint8_t)item.slot != slot || item.tool || item.tier == RELIC_TIER)
            {
                continue;
            }
            bool earlier = false;
            for (uint8_t z = 0; z < zone; z++)
            {
                earlier = earlier || zoneDrops(z, i);
            }
            bool own = stage.ownLevel > 0 && isZoneSetPiece(zone, i);
            if (!earlier && !own)
            {
                continue;
            }
            uint8_t level = own ? stage.ownLevel : stage.earlierLevel;
            double growth = 1 + itemGrowthPermille(level) / 1000.0;
            double score = (item.attack * 3.0 + item.defense * 2.0 + item.maxHp * 0.5) * growth;
            if (score > bestScore)
            {
                bestScore = score, best = i, bestLevel = level;
            }
        }
        if (best >= 0)
        {
            for (uint8_t l = 0; l < bestLevel; l++)
            {
                inventory.addDrop((uint8_t)best);
            }
            inventory.equip((uint8_t)best);
        }
    }

    // The stage's gear stays as it is: what drops in the fight is put back
    Inventory::Snapshot kit;
    inventory.save(kit);
    uint32_t kitRevision = inventory.getRevision();

    simNow = 1000;
    combat.enterZone(zone, simNow);
    uint32_t deaths = 0;
    bool wasDead = false;
    for (unsigned long end = simNow + minutes * 60000UL; simNow < end; simNow += 50)
    {
        game.update(simNow);
        combat.update(simNow);
        if (inventory.getRevision() != kitRevision)
        {
            inventory.load(kit);
            kitRevision = inventory.getRevision();
        }
        if (active && combat.getPhase() == CombatState::FIGHTING)
        {
            combat.strike(simNow);
            if (combat.getEnemyAttackRemaining(simNow) < 600 && combat.getEnemyAttackDamage() > 0)
            {
                combat.guard(simNow);
            }
        }
        bool dead = combat.getPhase() == CombatState::DEAD;
        deaths += dead && !wasDead;
        wasDead = dead;
    }
    double hours = minutes / 60.0;
    uint32_t kills = combat.getTotalKills();
    return {kills / hours, kills ? (double)deaths / kills : deaths, resolvePlayerStats(game, inventory)};
}

static bool outside(double value, double min, double max)
{
    return (min >= 0 && value < min) || (max >= 0 && value > max);
}

int main(int argc, char **argv)
{
    double factor = argc > 1 ? atof(argv[1]) : 4;
    unsigned long minutes = argc > 2 ? strtoul(argv[2], nullptr, 10) : 60;
    int only = argc > 3 ? atoi(argv[3]) : -1;
    int misses = 0;
    for (uint8_t zone = 1; zone <= ZONE_MOLTEN_CORE; zone++)
    {
        if (only >= 0 && zone != only)
        {
            continue;
        }
        printf("%s\n", ZONES[zone].name);
        for (const Stage &stage : STAGES)
        {
            Outcome a = fight(zone, stage, factor, minutes, true);
            Outcome i = fight(zone, stage, factor, minutes, false);
            bool miss = outside(a.kills, stage.activeKillsMin, stage.activeKillsMax) ||
                        outside(a.deaths, stage.activeDeathsMin, stage.activeDeathsMax) ||
                        outside(i.kills, stage.idleKillsMin, stage.idleKillsMax) ||
                        outside(i.deaths, stage.idleDeathsMin, stage.idleDeathsMax);
            misses += miss;
            printf("  %-18s ATK %5u DEF %5u HP %6u   active %5.0f kills/h %6.3f deaths/kill   idle %5.0f kills/h %6.3f deaths/kill%s\n",
                   stage.name, (unsigned)a.stats.attack, (unsigned)a.stats.defense, (unsigned)a.stats.maxHp,
                   a.kills, a.deaths, i.kills, i.deaths, miss ? "  << off the ladder" : "");
        }
    }
    printf(misses == 0 ? "every stage on the ladder\n" : "%d stages off the ladder\n", misses);
    return 0;
}
