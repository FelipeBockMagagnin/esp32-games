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
#include "../../src/game/Ascension.h"
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
static int runs = 1;
static bool ascendOnSuggest = false;

static bool isAwake(unsigned long t)
{
    return (t + START_OF_DAY) % DAY >= 8 * HOUR;
}

static bool isActive(unsigned long t)
{
    if (t < firstSession)
    {
        return true;
    }
    return isAwake(t) && ((t + START_OF_DAY) % HOUR) < sessionLength;
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
Ascension ascension;
GameState game(inventory, achievements, ascension);
CombatState combat(game, inventory);

static bool verbose = false;
static std::vector<std::string> summary;

// The longest the player spends awake without anything new (a zone change is not news),
// the measure of a run's dry stretches
static unsigned long dryAwake = 0, dryFrom = 0, longestDry = 0, longestDryFrom = 0;

// Awake time from a zone opening to it farming itself, the measure of a combat wall, and
// to farming itself without dying. All hold the awake clock plus one, so 0 means not yet
// in this run.
static unsigned long awakeClock = 0;
static unsigned long zoneOpenedAwake[ZONE_COUNT], zoneIdleAwake[ZONE_COUNT], zoneSafeAwake[ZONE_COUNT];
// Deaths a kill below which an idle fight counts as not dying
static const double SAFE_DEATHS_PER_KILL = 0.01;

static bool awardSeen[ACHIEVEMENT_COUNT];

// When each item first reached these levels, and the kills and time spent in each zone
static const uint8_t TRACKED_LEVELS[] = {1, 25, 50, 100};
static const uint8_t TRACKED_COUNT = sizeof(TRACKED_LEVELS) / sizeof(TRACKED_LEVELS[0]);
static unsigned long levelReachedAt[ITEM_COUNT][TRACKED_COUNT];
static unsigned long zoneTime[ZONE_COUNT][2];
static uint32_t zoneKillCount[ZONE_COUNT][2];

static void trackItems(unsigned long now)
{
    for (uint8_t i = 0; i < ITEM_COUNT; i++)
    {
        for (uint8_t l = 0; l < TRACKED_COUNT; l++)
        {
            if (levelReachedAt[i][l] == 0 && inventory.getItemLevel(i) >= TRACKED_LEVELS[l])
            {
                levelReachedAt[i][l] = now + 1;
            }
        }
    }
}

// Items in the order their zone drops them first, which is roughly the order they are met
static void printItemReport()
{
    printf("\nItems (hours since the start, - for not yet):\n  %-12s %-9s", "item", "tier");
    for (uint8_t l = 0; l < TRACKED_COUNT; l++)
    {
        printf("  Lv%-4u", (unsigned)TRACKED_LEVELS[l]);
    }
    printf("\n");
    for (uint8_t tier = 0; tier < ITEM_TIER_COUNT; tier++)
    {
        for (uint8_t i = 0; i < ITEM_COUNT; i++)
        {
            if (ITEMS[i].tier != tier)
            {
                continue;
            }
            printf("  %-12s %-9s", ITEMS[i].name, ITEMS[i].tool ? "tool" : ITEM_TIER_NAMES[tier]);
            for (uint8_t l = 0; l < TRACKED_COUNT; l++)
            {
                if (levelReachedAt[i][l])
                {
                    printf("  %6.1f", (levelReachedAt[i][l] - 1) / (double)HOUR);
                }
                else
                {
                    printf("  %6s", "-");
                }
            }
            printf("\n");
        }
    }
    printf("\nKills per hour:\n");
    for (uint8_t z = 0; z < ZONE_COUNT; z++)
    {
        for (uint8_t a = 0; a < 2; a++)
        {
            if (zoneTime[z][a] >= MIN)
            {
                double hours = zoneTime[z][a] / (double)HOUR;
                printf("  %-12s %-7s %6.1fh  %6.0f/h\n", ZONES[z].name, a ? "active" : "away", hours,
                       zoneKillCount[z][a] / hours);
            }
        }
    }
}

static void event(const std::string &what, bool milestone = false)
{
    std::string line = stamp(simNow) + "  " + what;
    if (what.rfind("zone", 0) != 0 && what.rfind("ARRIVE", 0) != 0)
    {
        dryAwake = 0, dryFrom = simNow;
    }
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
    case UpgradeTarget::AWARD_PRODUCTION:
        return gps * def.bonusPercent * game.getAchievements().getUnlockedCount() /
               (100.0 + game.getProductionBonusPercent());
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
            double cost = (double)game.getGoldUpgradeCost(i);
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
// amulets go by tier since what they grant is a spell. Tools never win this on stats, but
// a relic's perk is taken as worth the stats it gave up against its zone's tier.
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
            bool combatTool = inventory.getItemPerk(i, ItemPerk::ARMOR_BREAK) + inventory.getItemPerk(i, ItemPerk::WEAK_POINT) +
                                  inventory.getItemPerk(i, ItemPerk::FURY) > 0;
            score *= combatTool ? 1.3 : 1.0;
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

struct ZoneOutlook;
static ZoneOutlook outlook(uint8_t z, const Stats &s, bool active);
static bool farmsItself(uint8_t z);

// Loot found per second while away: the chance the worn drop perks give times the kills
// the auto-attack makes alone, 0 where the zone does not farm itself. A swap that trades
// attack for a drop perk can slow the kills more than it raises the chance.
static double farmRate(uint8_t z)
{
    if (!farmsItself(z))
    {
        return 0;
    }
    Stats s = resolvePlayerStats(game, inventory);
    const ZoneDef &zone = ZONES[z];
    double fight = 0;
    for (uint8_t e = 0; e < zone.enemyCount; e++)
    {
        const EnemyDef &en = zone.enemies[e];
        fight += en.maxHp / (damageThrough(s.attack, en.defense) / (AUTO_ATTACK_MS / 1000.0));
    }
    uint32_t cut = inventory.getPerkPercent(ItemPerk::RESPAWN_CUT);
    double respawn = RESPAWN_MS / 1000.0 * (100 - (cut < MAX_RESPAWN_CUT_PERCENT ? cut : MAX_RESPAWN_CUT_PERCENT)) / 100;
    return (100 + inventory.getPerkPercent(ItemPerk::DROP)) / (fight / zone.enemyCount + respawn);
}

// On leaving, the player swaps in whatever finds more loot an hour, slot by slot, then
// whatever pays more gold a kill at the same loot rate
static void equipFarmLoadout(uint8_t zone)
{
    for (uint8_t slot = 0; slot < EQUIP_SLOT_COUNT; slot++)
    {
        int8_t best = inventory.getEquipped((EquipSlot)slot);
        double bestRate = farmRate(zone);
        uint32_t bestGold = best == NO_ITEM ? 0 : inventory.getItemPerk(best, ItemPerk::KILL_GOLD);
        for (uint8_t i = 0; i < ITEM_COUNT; i++)
        {
            if ((uint8_t)ITEMS[i].slot != slot || !inventory.isOwned(i) || i == best)
            {
                continue;
            }
            inventory.equip(i);
            double rate = farmRate(zone);
            uint32_t gold = inventory.getItemPerk(i, ItemPerk::KILL_GOLD);
            if (rate > bestRate * 1.001 || (rate >= bestRate * 0.999 && rate > 0 && gold > bestGold))
            {
                best = i, bestRate = rate, bestGold = gold;
            }
        }
        if (best != NO_ITEM)
        {
            inventory.equip((uint8_t)best);
        }
        else
        {
            inventory.unequip((EquipSlot)slot);
        }
    }
}

// How a zone would go with these stats: can the auto attack (or the strike) get through
// every enemy, and roughly how many deaths per kill it costs. An auto attack that only
// scratches an enemy does not farm the zone: past this many seconds a kill on average,
// the drops would take days.
static const double MAX_IDLE_KILL_S = 60;

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
    double deaths = 0, idleFight = 0;
    for (uint8_t e = 0; e < zone.enemyCount; e++)
    {
        const EnemyDef &en = zone.enemies[e];
        double autoHit = damageThrough(s.attack, en.defense);
        double strikeHit = damageThrough(s.attack * STRIKE_MULTIPLIER, en.defense);
        o.idle = o.idle && autoHit > 0;
        idleFight += autoHit > 0 ? en.maxHp / (autoHit / (AUTO_ATTACK_MS / 1000.0)) : 0;
        o.strike = o.strike && strikeHit > 0;
        double dps = autoHit / (AUTO_ATTACK_MS / 1000.0) + (active ? strikeHit / (STRIKE_COOLDOWN_MS / 1000.0) : 0);
        if (dps <= 0)
        {
            deaths += 99;
            continue;
        }
        double fight = en.maxHp / dps;
        double hurt = hurtThrough(en.attack, s.defense);
        if (active)
        {
            hurt *= 0.7; // Timed guards
        }
        double netHurt = hurt / (en.attackIntervalMs / 1000.0) - s.regen / 10.0;
        deaths += netHurt > 0 ? fight * netHurt / s.maxHp : 0;
    }
    o.deathsPerKill = deaths / zone.enemyCount;
    o.idle = o.idle && idleFight / zone.enemyCount <= MAX_IDLE_KILL_S;
    return o;
}

// What a player arrives with: the best piece per slot from the zones before this one,
// either as owned (at its level) or, for one who moved on at once, every piece at level 1
static Stats arrivalKit(uint8_t zone, bool levelOne)
{
    Inventory kit;
    for (uint8_t slot = 0; slot < EQUIP_SLOT_COUNT; slot++)
    {
        int best = -1;
        double bestScore = -1;
        for (uint8_t i = 0; i < ITEM_COUNT; i++)
        {
            const ItemDef &item = ITEMS[i];
            bool earlier = false;
            for (uint8_t z = 0; z < zone && !earlier; z++)
            {
                earlier = enemiesDropItem(ZONES[z].enemies, ZONES[z].enemyCount, i);
            }
            if ((uint8_t)item.slot != slot || item.tool || item.tier == RELIC_TIER || !earlier ||
                (!levelOne && !inventory.isOwned(i)))
            {
                continue;
            }
            Stats st = levelOne ? Stats{item.attack, item.defense, item.maxHp, 0} : inventory.getItemStats(i);
            double score = st.attack * 3.0 + st.defense * 2.0 + st.maxHp * 0.5;
            if (score > bestScore)
            {
                bestScore = score, best = i;
            }
        }
        if (best >= 0)
        {
            for (uint8_t level = levelOne ? 1 : inventory.getItemLevel((uint8_t)best); level > 0; level--)
            {
                kit.addDrop((uint8_t)best);
            }
            kit.equip((uint8_t)best);
        }
    }
    return resolvePlayerStats(game, kit);
}

// How a zone meets these stats, against the balance rule (CombatConfig.h): the auto attack
// blocked by every regular, the strike through all, and a regular's hit worth guarding
static std::string judge(uint8_t z, const Stats &s)
{
    const ZoneDef &zone = ZONES[z];
    unsigned autoThrough = 0, strikeThrough = 0, regulars = 0;
    double minHurt = 1e9, maxHurt = 0;
    for (uint8_t e = 0; e < zone.enemyCount; e++)
    {
        const EnemyDef &en = zone.enemies[e];
        strikeThrough += damageThrough(s.attack * STRIKE_MULTIPLIER, en.defense) > 0;
        if (en.boss)
        {
            continue;
        }
        regulars++;
        autoThrough += damageThrough(s.attack, en.defense) > 0;
        double hurt = 100.0 * hurtThrough(en.attack, s.defense) / s.maxHp;
        minHurt = hurt < minHurt ? hurt : minHurt;
        maxHurt = hurt > maxHurt ? hurt : maxHurt;
    }
    char buf[160];
    snprintf(buf, sizeof(buf), "ATK %u DEF %u HP %u: auto %u/%u, strike %u/%u, hit %.0f-%.0f%%%s",
             (unsigned)s.attack, (unsigned)s.defense, (unsigned)s.maxHp, autoThrough, regulars, strikeThrough,
             zone.enemyCount, minHurt, maxHurt,
             strikeThrough < zone.enemyCount ? "  << WALL" : autoThrough > 0 || maxHurt < 3 ? "  << TOO EASY" : "");
    return buf;
}

static bool farmsItself(uint8_t z)
{
    ZoneOutlook o = outlook(z, resolvePlayerStats(game, inventory), false);
    return o.idle && o.deathsPerKill < 0.7;
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
    combat.strike(now);

    unsigned long hitIn = combat.getEnemyAttackRemaining(now);
    bool hitComing = hitIn < 600 && combat.getEnemyAttackDamage() > 0;
    if (hitComing && combat.getGuardCooldownRemaining(now) == 0)
    {
        combat.guard(now);
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
                combat.castSpell(now);
            }
            break;
        case SpellEffect::STUN:
        case SpellEffect::WARD:
            if (hitComing && !combat.isGuarding(now))
            {
                combat.castSpell(now);
            }
            break;
        case SpellEffect::MIDAS:
        case SpellEffect::SEEK:
            combat.castSpell(now);
            break;
        }
    }
}

static void printSince(const char *what, const unsigned long *reached)
{
    printf("  Awake hours from opening to %s:", what);
    for (uint8_t z = 1; z < ZONE_COUNT; z++)
    {
        if (zoneOpenedAwake[z] == 0)
        {
            continue;
        }
        if (reached[z] == 0)
        {
            printf("  %s - (%.1f so far)", ZONES[z].name, (awakeClock + 1 - zoneOpenedAwake[z]) / (double)HOUR);
        }
        else
        {
            printf("  %s %.1f", ZONES[z].name, (reached[z] - zoneOpenedAwake[z]) / (double)HOUR);
        }
    }
    printf("\n");
}

static void printMilestones()
{
    printf("\nMilestones:\n");
    for (const std::string &line : summary)
    {
        printf("  %s\n", line.c_str());
    }
    summary.clear();
    printf("  Longest awake stretch with nothing new: %.1fh from %s\n", longestDry / (double)HOUR,
           stamp(longestDryFrom).c_str());
    dryAwake = longestDry = 0;
    printSince("farming itself", zoneIdleAwake);
    printSince("farming without dying", zoneSafeAwake);
    for (uint8_t z = 0; z < ZONE_COUNT; z++)
    {
        zoneOpenedAwake[z] = zoneIdleAwake[z] = zoneSafeAwake[z] = 0;
    }
}

// What the game does on Ascend, minus the reboot: bank the points, keep the items and
// awards, and load a new game's (zeroed) snapshots over the run. Talents that open content
// are bought first, as the maps are in the shop; the rest of the points go to the cheapest
// level on offer, which spreads them over the roots before going deep.
static void ascendRun(unsigned long runStart)
{
    uint32_t gained = ascension.getPendingPoints(game.getTotalGoldEarned());
    ascension.ascend(game.getTotalGoldEarned());
    for (;;)
    {
        int best = -1;
        for (uint8_t t = 0; t < TALENT_COUNT; t++)
        {
            // Themes change nothing the simulator measures
            if (TALENTS[t].effect != TalentEffect::THEMES && ascension.canBuyTalent(t) &&
                (best < 0 || ascension.getTalentCost(t) < ascension.getTalentCost(best)))
            {
                best = t;
            }
        }
        for (uint8_t t : {TALENT_OLD_MAPS, TALENT_DEEP_DELVER, TALENT_RELIC_SENSE})
        {
            if (ascension.isTalentOpen(t) && !ascension.isTalentMaxed(t))
            {
                // Saves up for it rather than spending on anything else
                best = ascension.canBuyTalent(t) ? t : -1;
                break;
            }
        }
        if (best < 0)
        {
            break;
        }
        ascension.buyTalent((uint8_t)best);
    }

    GameState::Snapshot freshGame = {};
    CombatState::Snapshot freshCombat = {};
    game.load(freshGame);
    combat.load(freshCombat);

    printf("\n==== %s  ASCENDED +%u points (%u earned, gold/s x%.2f from ascension) on run day %.1f. Talents:",
           stamp(simNow).c_str(), (unsigned)gained, (unsigned)ascension.getPointsEarned(),
           (100 + game.getAscensionProductionPercent()) / 100.0, (simNow - runStart) / (double)DAY);
    for (uint8_t t = 0; t < TALENT_COUNT; t++)
    {
        printf(" %s %u%s", TALENTS[t].name, (unsigned)ascension.getTalentLevel(t), t + 1 < TALENT_COUNT ? "," : "\n");
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
        else if (a == "--runs" && i + 1 < argc) runs = atoi(argv[++i]);
        else if (a == "--suggest") ascendOnSuggest = true;
    }

    unsigned long runLength = (unsigned long)(days * DAY);
    unsigned long runStart = 0;
    int runsDone = 0;
    double clickDebt = 0;
    int zoneShown = -2;
    uint8_t zoneUnlockedSeen = 1;
    unsigned long arrivalDue[ZONE_COUNT] = {};
    bool idleReach[ZONE_COUNT] = {};
    bool safeReach[ZONE_COUNT] = {};
    bool wasDead = false;
    uint32_t deaths = 0;
    uint8_t itemsSeen = inventory.countOwnedItems();
    unsigned long nextReport = 6 * HOUR;
    unsigned long nextChoice = 0;
    bool wasActive = true;
    uint32_t killsSeen = 0;

    unsigned long veinSeenAt = 0;
    bool veinWasVisible = false;
    double luckyGold = 0;

    // A run lasts --days, or with --suggest ends at the first check-in once the game
    // suggests ascending
    for (simNow = 0;; simNow += TICK)
    {
        unsigned long now = simNow;
        bool suggested = ascendOnSuggest && isActive(now) && game.isAscendSuggested();
        if (now > runStart && (now - runStart >= runLength || suggested))
        {
            if (++runsDone == runs)
            {
                break;
            }
            printMilestones();
            ascendRun(runStart);
            runStart = now;
            zoneShown = -2;
            zoneUnlockedSeen = 1;
            for (uint8_t z = 0; z < ZONE_COUNT; z++)
            {
                idleReach[z] = safeReach[z] = false;
            }
        }
        bool active = isActive(now);
        if (isAwake(now))
        {
            awakeClock += TICK;
            dryAwake += TICK;
            if (dryAwake > longestDry)
            {
                longestDry = dryAwake, longestDryFrom = dryFrom;
            }
        }

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
                game.mine(levelled, now);
                clickDebt -= 1;
            }
        }

        game.update(now);
        combat.update(now);
        achievements.evaluate(now, game, inventory, combat);
        for (uint8_t i = 0; i < ACHIEVEMENT_COUNT; i++)
        {
            if (achievements.isUnlocked(i) && !awardSeen[i])
            {
                awardSeen[i] = true;
                event(std::string("AWARD    ") + ACHIEVEMENTS[i].name, ACHIEVEMENTS[i].reward != AchReward::PRODUCTION);
            }
        }

        if (active)
        {
            fight(now);
        }

        // Decisions every few seconds while present, and once on leaving: the gear that
        // fights best, the zone, then the loot loadout that zone still farms in
        bool justLeft = wasActive && !active;
        wasActive = active;
        if (justLeft)
        {
            nextChoice = now;
        }
        if ((long)(now - nextChoice) >= 0 && (active || justLeft))
        {
            nextChoice = now + 2 * SEC;
            shop();
            equipBest();
            if (game.getBuildingLevel(BUILDING_DRILL) > 0)
            {
                int z = bestZone(active);
                if (z < 0 && combat.isInZone())
                {
                    combat.leaveZone();
                }
                else if (z >= 0 && (!combat.isInZone() || combat.getCurrentZone() != z))
                {
                    combat.enterZone((uint8_t)z, now);
                }
                if (justLeft && combat.isInZone())
                {
                    equipFarmLoadout(combat.getCurrentZone());
                }
                int shown = combat.isInZone() ? combat.getCurrentZone() : -1;
                if (shown != zoneShown)
                {
                    zoneShown = shown;
                    event(std::string(shown < 0 ? "zone     none reachable" : std::string("zone     fighting in ") + ZONES[shown].name) +
                          (active ? "" : " (away)"));
                }
            }
        }

        // A zone is judged once the check-in that opened it is over, so the combat upgrades
        // bought beside its map count, against the gear worn and against a level-1 kit
        for (uint8_t z = 1; z < ZONE_COUNT; z++)
        {
            if (arrivalDue[z] != 0 && (!active || (long)(now - arrivalDue[z]) >= 0))
            {
                event(std::string("ARRIVE   ") + ZONES[z].name + "  owned " + judge(z, arrivalKit(z, false)) +
                          "; level-1 kit " + judge(z, arrivalKit(z, true)),
                      true);
                arrivalDue[z] = 0;
            }
        }

        // Milestones
        while (zoneUnlockedSeen < ZONE_COUNT && combat.isZoneUnlocked(zoneUnlockedSeen))
        {
            arrivalDue[zoneUnlockedSeen] = now + 10 * MIN;
            Stats s = resolvePlayerStats(game, inventory);
            uint8_t prevZone = zoneUnlockedSeen - 1;
            unsigned prevOwned = 0, prevLevels = 0, prevTotal = 0;
            for (uint8_t i = 0; i < ITEM_COUNT; i++)
            {
                if (isZoneSetPiece(prevZone, i))
                {
                    prevTotal++;
                    prevOwned += inventory.isOwned(i);
                    prevLevels += inventory.getItemLevel(i);
                }
            }
            char buf[160];
            snprintf(buf, sizeof(buf), "UNLOCKED %s  arrive ATK %u DEF %u HP %u, %s set %u/%u avg Lv %.1f",
                     ZONES[zoneUnlockedSeen].name, (unsigned)s.attack, (unsigned)s.defense, (unsigned)s.maxHp,
                     ZONES[prevZone].name, prevOwned, prevTotal, prevTotal ? prevLevels / (double)prevTotal : 0.0);
            event(buf, true);
            zoneOpenedAwake[zoneUnlockedSeen] = awakeClock + 1;
            zoneUnlockedSeen++;
        }
        if (now % MIN == 0)
        {
            Stats s = resolvePlayerStats(game, inventory);
            for (uint8_t z = 0; z < ZONE_COUNT; z++)
            {
                if (!combat.isZoneUnlocked(z))
                {
                    continue;
                }
                ZoneOutlook o = outlook(z, s, false);
                std::string at = " (ATK " + std::to_string(s.attack) + " DEF " + std::to_string(s.defense) + " HP " +
                                 std::to_string(s.maxHp) + ")";
                if (!idleReach[z] && o.idle)
                {
                    idleReach[z] = true;
                    zoneIdleAwake[z] = awakeClock + 1;
                    event(std::string("IDLE     ") + ZONES[z].name + " farms itself" + at, true);
                }
                if (!safeReach[z] && o.idle && o.deathsPerKill < SAFE_DEATHS_PER_KILL)
                {
                    safeReach[z] = true;
                    zoneSafeAwake[z] = awakeClock + 1;
                    event(std::string("SAFE     ") + ZONES[z].name + " farms itself without dying" + at, true);
                }
            }
        }
        trackItems(now);
        if (combat.isInZone())
        {
            uint8_t z = combat.getCurrentZone();
            zoneTime[z][active] += TICK;
            // An ascension zeroes the kills
            zoneKillCount[z][active] += combat.getTotalKills() >= killsSeen ? combat.getTotalKills() - killsSeen : 0;
        }
        killsSeen = combat.getTotalKills();

        if (inventory.countOwnedItems() != itemsSeen)
        {
            itemsSeen = inventory.countOwnedItems();
            event("item     found #" + std::to_string(itemsSeen) + " " + ITEMS[combat.getLastDrops().itemId].name);
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
                   "  items %u (avg Lv %.1f)  kills %u  deaths %u  veins %u  ascend +%u\n",
                   stamp(now).c_str(), fmt(game.getProductionPerSecond() / (double)GOLD_SCALE).c_str(),
                   fmt((double)game.getGold()).c_str(), (unsigned)game.getTotalBuildingLevels(),
                   (unsigned)game.getUpgradesBoughtCount(), (unsigned)GOLD_UPGRADE_COUNT,
                   (unsigned)game.getMiningLevel(), (unsigned)s.attack, (unsigned)s.defense, (unsigned)s.maxHp,
                   owned, owned ? levels / (double)owned : 0.0, (unsigned)combat.getTotalKills(), deaths,
                   (unsigned)game.getVeinsCaught(), (unsigned)ascension.getPendingPoints(game.getTotalGoldEarned()));
        }
    }

    printMilestones();
    printItemReport();
    printf("\nBuildings:");
    for (uint8_t b = 0; b < BUILDING_COUNT; b++)
    {
        printf(" %s %u%s", BUILDINGS[b].name, (unsigned)game.getBuildingLevel(b), b + 1 < BUILDING_COUNT ? "," : "\n");
    }
    printf("Gold veins: %u caught, %u appeared; Lucky paid %s in total\n", (unsigned)game.getVeinsCaught(),
           (unsigned)game.getVeinSpawns(), fmt(luckyGold).c_str());
    printf("Achievements: %u/%u\n", (unsigned)achievements.getUnlockedCount(), (unsigned)ACHIEVEMENT_COUNT);
    printf("Ascending now banks %u points (run gold %s)\n",
           (unsigned)ascension.getPendingPoints(game.getTotalGoldEarned()), fmt((double)game.getTotalGoldEarned()).c_str());
    return 0;
}
