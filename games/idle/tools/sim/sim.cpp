// Balance simulator: runs the real game logic from src/game/ on a PC against a simulated
// clock and a scripted player, and prints when each milestone lands. Build and run with
// tools/sim/run.sh; every number it reports comes from the same tables the ESP32 uses.
//
// The player: plays actively for the first half hour, then checks in for a few minutes
// every hour while awake (08:00 to 24:00) and sleeps the rest. While present it mines,
// buys whatever pays back fastest, fights in the best zone it can survive, strikes,
// guards and casts on cue, and equips its best gear. While away the fight runs on the
// auto attack alone, in the best zone that farms itself.
#include <Arduino.h>
#include <cmath>
#include <random>
#include <string>
#include <vector>

#include "../../src/game/GameState.h"
#include "../../src/game/Inventory.h"
#include "../../src/game/CombatState.h"
#include "../../src/game/Achievements.h"
#include "../../src/game/Stats.h"

// ---- Arduino stubs --------------------------------------------------------------------
static unsigned long simNow = 0;
static std::mt19937 rng(1);
unsigned long millis() { return simNow; }
long random(long max) { return max <= 0 ? 0 : (long)(rng() % (unsigned long)max); }
long random(long min, long max) { return min + random(max - min); }

// ---- Player model ---------------------------------------------------------------------
static const unsigned long SEC = 1000, MIN = 60 * SEC, HOUR = 60 * MIN, DAY = 24 * HOUR;
static const unsigned long TICK = 100;
static const unsigned long START_OF_DAY = 8 * HOUR; // The sim starts at 08:00
static unsigned long firstSession = 30 * MIN;
static unsigned long sessionLength = 5 * MIN; // Per waking hour
static double clicksPerSecond = 3.0;
static double days = 3.0;

static bool isActive(unsigned long t)
{
    if (t < firstSession)
    {
        return true;
    }
    unsigned long clock = (t + START_OF_DAY) % DAY;
    bool awake = clock >= 8 * HOUR;
    return awake && (clock % HOUR) < sessionLength;
}

static std::string stamp(unsigned long t)
{
    unsigned long day = (t + START_OF_DAY) / DAY + 1;
    unsigned long clock = (t + START_OF_DAY) % DAY;
    char buf[32];
    snprintf(buf, sizeof(buf), "d%lu %02lu:%02lu (+%5.1fh)", day, clock / HOUR, (clock % HOUR) / MIN,
             t / (double)HOUR);
    return buf;
}

static std::string fmt(double v)
{
    const char *suffix[] = {"", "K", "M", "B", "T", "Q", "Qi"};
    int i = 0;
    while (v >= 1000 && i < 6)
    {
        v /= 1000;
        i++;
    }
    char buf[24];
    snprintf(buf, sizeof(buf), i ? "%.1f%s" : "%.0f%s", v, suffix[i]);
    return buf;
}

// ---- Game objects, in the same dependency order as main.cpp ---------------------------
Achievements achievements;
Inventory inventory;
GameState game(inventory, achievements);
CombatState combat(game, inventory);

static bool verbose = false;
static std::vector<std::string> summary;

static void event(const std::string &what, bool milestone = false)
{
    std::string line = stamp(simNow) + "  " + what;
    if (verbose)
    {
        printf("%s\n", line.c_str());
    }
    if (milestone)
    {
        summary.push_back(line);
    }
}

// Paybacks are judged on production without a running frenzy, so a purchase made during
// one is not mistaken for seven times as good
static double baseGps()
{
    double gps = game.getProductionPerSecond() / (double)GOLD_SCALE;
    return game.getActiveBuff() == VeinEffect::FRENZY ? gps / VEIN_FRENZY_MULTIPLIER : gps;
}

// What one more unit of each purchase adds, in gold/s, so purchases can be compared by
// how fast they pay for themselves. Clicks are valued at the player's average click rate
// over a day, which is small: click upgrades matter early and fade, as intended.
static double averageClickRate()
{
    double activeShare = sessionLength / (double)HOUR * 16.0 / 24.0;
    return clicksPerSecond * activeShare;
}

static double upgradeGain(uint8_t id)
{
    const GoldUpgradeDef &def = GOLD_UPGRADES[id];
    double gps = baseGps();
    switch (def.target)
    {
    case UpgradeTarget::PRODUCTION:
        return gps * def.bonusPercent / (100.0 + game.getProductionBonusPercent());
    case UpgradeTarget::BUILDING_BOOST:
        return game.getBuildingProduction(def.reqBuilding) / (double)GOLD_SCALE;
    case UpgradeTarget::CLICK:
        return ORE_TIERS[game.getOreTier()].clickAmount * def.bonusPercent / 100.0 * averageClickRate();
    case UpgradeTarget::CLICK_PRODUCTION:
        return gps * def.bonusPercent / 100.0 * averageClickRate();
    default:
        return 0;
    }
}

// Bought as soon as they are affordable: they gate content or feed the gold veins, whose
// worth the payback comparison below has no way to measure
static bool isCombatUpgrade(UpgradeTarget t)
{
    return t == UpgradeTarget::ATTACK || t == UpgradeTarget::DEFENSE || t == UpgradeTarget::MAX_HP ||
           t == UpgradeTarget::UNLOCK_ZONE || t == UpgradeTarget::VEIN_FREQUENCY || t == UpgradeTarget::VEIN_POWER;
}

static int buildingsOwnedFirst[BUILDING_COUNT];
static int upgradesBoughtAt[GOLD_UPGRADE_COUNT];

static void shop()
{
    for (int guard = 0; guard < 500; guard++)
    {
        double gold = (double)game.getGold();

        // Combat upgrades and maps are bought on sight: they gate content, not income
        bool bought = false;
        for (uint8_t i = 0; i < GOLD_UPGRADE_COUNT && !bought; i++)
        {
            if (isCombatUpgrade(GOLD_UPGRADES[i].target) && game.canAffordGoldUpgrade(i))
            {
                game.buyGoldUpgrade(i);
                event(std::string("upgrade  ") + GOLD_UPGRADES[i].name);
                bought = true;
            }
        }
        if (bought)
        {
            continue;
        }

        // Otherwise the best payback among everything on offer; if that is not affordable
        // yet, save for it rather than spending on something worse
        double best = 1e300;
        int bestKind = -1, bestId = -1;
        double bestCost = 0;
        for (uint8_t b = 0; b < BUILDING_COUNT; b++)
        {
            // A building is on offer once the one before it is owned, as on the shop screen
            if (b > 0 && game.getBuildingLevel(b - 1) == 0)
            {
                break;
            }
            double cost = (double)game.getBuildingCost(b);
            double gain = game.getBuildingProductionPerLevel(b) / (double)GOLD_SCALE;
            double payback = cost / gain;
            if (payback < best)
            {
                best = payback, bestKind = 0, bestId = b, bestCost = cost;
            }
        }
        for (uint8_t i = 0; i < GOLD_UPGRADE_COUNT; i++)
        {
            if (!game.isGoldUpgradeUnlocked(i) || game.isGoldUpgradeBought(i) ||
                isCombatUpgrade(GOLD_UPGRADES[i].target))
            {
                continue;
            }
            double gain = upgradeGain(i);
            if (gain <= 0)
            {
                continue;
            }
            double cost = (double)GOLD_UPGRADES[i].cost;
            double payback = cost / gain;
            if (payback < best)
            {
                best = payback, bestKind = 1, bestId = i, bestCost = cost;
            }
        }
        if (bestKind < 0 || bestCost > gold)
        {
            return;
        }
        if (bestKind == 0)
        {
            game.buyBuilding((uint8_t)bestId);
            if (game.getBuildingLevel((uint8_t)bestId) == 1)
            {
                event(std::string("BUILDING ") + BUILDINGS[bestId].name + "  (gold/s " +
                      fmt(game.getProductionPerSecond() / (double)GOLD_SCALE) + ")", true);
            }
        }
        else
        {
            game.buyGoldUpgrade((uint8_t)bestId);
            upgradesBoughtAt[bestId] = 1;
            event(std::string("upgrade  ") + GOLD_UPGRADES[bestId].name);
        }
    }
}

// Best owned item per slot, weighting attack above defense above health and regen;
// amulets go by tier since what they grant is a spell
static void equipBest()
{
    for (uint8_t slot = 0; slot < EQUIP_SLOT_COUNT; slot++)
    {
        int best = -1;
        double bestScore = -1;
        for (uint8_t i = 0; i < ITEM_COUNT; i++)
        {
            if ((uint8_t)ITEMS[i].slot != slot || !inventory.isOwned(i))
            {
                continue;
            }
            Stats s = inventory.getItemStats(i);
            double score = s.attack * 3.0 + s.defense * 2.0 + s.maxHp * 0.5 + s.regen * 0.5 + ITEMS[i].tier * 1000.0 * (slot == (uint8_t)EquipSlot::AMULET);
            if (score > bestScore)
            {
                bestScore = score, best = i;
            }
        }
        if (best >= 0 && inventory.getEquipped((EquipSlot)slot) != best)
        {
            inventory.equip((uint8_t)best);
        }
    }
}

// How a zone would go with these stats: can the auto attack (or the strike) get through
// every enemy, and roughly how many deaths per kill it costs
struct ZoneOutlook
{
    bool idle;
    bool strike;
    double deathsPerKill;
};

static ZoneOutlook outlook(uint8_t z, const Stats &s, bool active)
{
    ZoneOutlook o{true, true, 0};
    const ZoneDef &zone = ZONES[z];
    double deaths = 0;
    for (uint8_t e = 0; e < zone.enemyCount; e++)
    {
        const EnemyDef &en = zone.enemies[e];
        double autoHit = damageThrough(s.attack, en.defense);
        double strikeHit = damageThrough(s.attack * STRIKE_MULTIPLIER, en.defense);
        o.idle = o.idle && autoHit > 0;
        o.strike = o.strike && strikeHit > 0;
        double dps = autoHit / (AUTO_ATTACK_MS / 1000.0) + (active ? strikeHit / (STRIKE_COOLDOWN_MS / 1000.0) : 0);
        if (dps <= 0)
        {
            deaths += 99;
            continue;
        }
        double fight = en.maxHp / dps;
        double hurt = damageThrough(en.attack, s.defense);
        if (active)
        {
            hurt *= 0.7; // Timed guards
        }
        double netHurt = hurt / (en.attackIntervalMs / 1000.0) - s.regen / 10.0;
        deaths += netHurt > 0 ? fight * netHurt / s.maxHp : 0;
    }
    o.deathsPerKill = deaths / zone.enemyCount;
    return o;
}

static int bestZone(bool active)
{
    Stats s = resolvePlayerStats(game, inventory);
    int best = -1;
    for (uint8_t z = 0; z < ZONE_COUNT; z++)
    {
        if (!combat.isZoneUnlocked(z))
        {
            continue;
        }
        ZoneOutlook o = outlook(z, s, active);
        bool reach = active ? o.strike : o.idle;
        if (reach && o.deathsPerKill < (active ? 1.5 : 0.7))
        {
            best = z;
        }
    }
    return best;
}

static void fight(unsigned long now)
{
    if (!combat.isInZone() || combat.getPhase() != CombatState::FIGHTING)
    {
        return;
    }
    combat.strike();

    unsigned long hitIn = combat.getEnemyAttackRemaining(now);
    bool hitComing = hitIn < 600 && combat.getEnemyAttackDamage() > 0;
    if (hitComing && combat.getGuardCooldownRemaining(now) == 0)
    {
        combat.guard();
    }

    const SpellDef *spell = combat.getSpell();
    if (spell != nullptr && combat.getSpellCooldownRemaining(now) == 0)
    {
        const Stats &s = combat.getPlayerStats();
        switch (spell->effect)
        {
        case SpellEffect::HEAL:
            if (combat.getPlayerHp() < s.maxHp / 2)
            {
                combat.castSpell();
            }
            break;
        case SpellEffect::STUN:
        case SpellEffect::WARD:
            if (hitComing && !combat.isGuarding(now))
            {
                combat.castSpell();
            }
            break;
        case SpellEffect::MIDAS:
            combat.castSpell();
            break;
        }
    }
}

int main(int argc, char **argv)
{
    for (int i = 1; i < argc; i++)
    {
        std::string a = argv[i];
        if (a == "--days" && i + 1 < argc) days = atof(argv[++i]);
        else if (a == "--log") verbose = true;
        else if (a == "--seed" && i + 1 < argc) rng.seed(atoi(argv[++i]));
        else if (a == "--session" && i + 1 < argc) sessionLength = (unsigned long)(atof(argv[++i]) * MIN);
        else if (a == "--cps" && i + 1 < argc) clicksPerSecond = atof(argv[++i]);
    }

    unsigned long end = (unsigned long)(days * DAY);
    double clickDebt = 0;
    int zoneShown = -2;
    uint8_t zoneUnlockedSeen = 1;
    bool idleReach[ZONE_COUNT] = {};
    bool wasDead = false;
    uint32_t deaths = 0;
    uint8_t itemsSeen = inventory.countOwnedItems();
    unsigned long nextReport = 6 * HOUR;
    unsigned long nextChoice = 0;

    unsigned long veinSeenAt = 0;
    bool veinWasVisible = false;
    double luckyGold = 0;

    for (simNow = 0; simNow < end; simNow += TICK)
    {
        unsigned long now = simNow;
        bool active = isActive(now);

        // Gold veins: a present player catches one a couple of seconds after it shows
        if (game.isVeinVisible() && !veinWasVisible)
        {
            veinSeenAt = now;
        }
        if (game.isVeinVisible() && active && now - veinSeenAt >= 2 * SEC)
        {
            if (game.catchVein(now) == VeinEffect::LUCKY)
            {
                luckyGold += (double)game.getLastLuckyGold();
            }
        }
        veinWasVisible = game.isVeinVisible();

        if (active)
        {
            clickDebt += clicksPerSecond * TICK / 1000.0;
            while (clickDebt >= 1)
            {
                bool levelled;
                game.mine(levelled);
                clickDebt -= 1;
            }
        }

        game.update(now);
        combat.update(now);
        achievements.evaluate(now, game, inventory, combat);

        if (active)
        {
            fight(now);
        }

        // Decisions every few seconds while present, and once on leaving
        if ((long)(now - nextChoice) >= 0)
        {
            nextChoice = now + (active ? 2 * SEC : MIN);
            if (active)
            {
                shop();
                equipBest();
            }
            if (game.getBuildingLevel(BUILDING_DRILL) > 0)
            {
                int z = bestZone(active);
                if (z < 0 && combat.isInZone())
                {
                    combat.leaveZone();
                }
                else if (z >= 0 && (!combat.isInZone() || combat.getCurrentZone() != z))
                {
                    combat.enterZone((uint8_t)z);
                }
                int shown = combat.isInZone() ? combat.getCurrentZone() : -1;
                if (shown != zoneShown && active)
                {
                    zoneShown = shown;
                    event(shown < 0 ? "zone     none reachable" : std::string("zone     fighting in ") + ZONES[shown].name);
                }
            }
        }

        // Milestones
        while (zoneUnlockedSeen < ZONE_COUNT && combat.isZoneUnlocked(zoneUnlockedSeen))
        {
            Stats s = resolvePlayerStats(game, inventory);
            unsigned prevOwned = 0, prevLevels = 0, prevTotal = 0;
            for (uint8_t i = 0; i < ITEM_COUNT; i++)
            {
                if (ITEMS[i].tier == zoneUnlockedSeen - 1)
                {
                    prevTotal++;
                    prevOwned += inventory.isOwned(i);
                    prevLevels += inventory.getItemLevel(i);
                }
            }
            char buf[160];
            snprintf(buf, sizeof(buf), "UNLOCKED %s  arrive ATK %u DEF %u HP %u, tier %u items %u/%u avg Lv %.1f",
                     ZONES[zoneUnlockedSeen].name, (unsigned)s.attack, (unsigned)s.defense, (unsigned)s.maxHp,
                     (unsigned)(zoneUnlockedSeen - 1), prevOwned, prevTotal, prevOwned ? prevLevels / (double)prevOwned : 0.0);
            event(buf, true);
            zoneUnlockedSeen++;
        }
        if (now % MIN == 0)
        {
            Stats s = resolvePlayerStats(game, inventory);
            for (uint8_t z = 0; z < ZONE_COUNT; z++)
            {
                if (!idleReach[z] && combat.isZoneUnlocked(z) && outlook(z, s, false).idle)
                {
                    idleReach[z] = true;
                    event(std::string("IDLE     ") + ZONES[z].name + " farms itself (ATK " + std::to_string(s.attack) +
                          " DEF " + std::to_string(s.defense) + " HP " + std::to_string(s.maxHp) + ")", true);
                }
            }
        }
        if (inventory.countOwnedItems() != itemsSeen)
        {
            itemsSeen = inventory.countOwnedItems();
            event("item     found #" + std::to_string(itemsSeen) + " " + ITEMS[combat.getLastDrop()].name);
        }
        bool dead = combat.getPhase() == CombatState::DEAD;
        deaths += dead && !wasDead;
        wasDead = dead;

        if (now >= nextReport)
        {
            nextReport += 12 * HOUR;
            Stats s = resolvePlayerStats(game, inventory);
            unsigned levels = 0, owned = 0;
            for (uint8_t i = 0; i < ITEM_COUNT; i++)
            {
                levels += inventory.getItemLevel(i);
                owned += inventory.isOwned(i);
            }
            printf("---- %s  gold/s %s  bank %s  buildings %u  upgrades %u/%u  mining Lv %u  ATK %u DEF %u HP %u"
                   "  items %u (avg Lv %.1f)  kills %u  deaths %u  veins %u\n",
                   stamp(now).c_str(), fmt(game.getProductionPerSecond() / (double)GOLD_SCALE).c_str(),
                   fmt((double)game.getGold()).c_str(), (unsigned)game.getTotalBuildingLevels(),
                   (unsigned)game.getUpgradesBoughtCount(), (unsigned)GOLD_UPGRADE_COUNT,
                   (unsigned)game.getMiningLevel(), (unsigned)s.attack, (unsigned)s.defense, (unsigned)s.maxHp,
                   owned, owned ? levels / (double)owned : 0.0, (unsigned)combat.getTotalKills(), deaths,
                   (unsigned)game.getVeinsCaught());
        }
    }

    printf("\nMilestones:\n");
    for (const std::string &line : summary)
    {
        printf("  %s\n", line.c_str());
    }
    printf("\nBuildings:");
    for (uint8_t b = 0; b < BUILDING_COUNT; b++)
    {
        printf(" %s %u%s", BUILDINGS[b].name, (unsigned)game.getBuildingLevel(b), b + 1 < BUILDING_COUNT ? "," : "\n");
    }
    printf("Gold veins: %u caught, %u appeared; Lucky paid %s in total\n", (unsigned)game.getVeinsCaught(),
           (unsigned)game.getVeinSpawns(), fmt(luckyGold).c_str());
    printf("Achievements: %u/%u\n", (unsigned)achievements.getUnlockedCount(), (unsigned)ACHIEVEMENT_COUNT);
    return 0;
}
