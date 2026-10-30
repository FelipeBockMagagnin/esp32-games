// Balance simulator: plays the real src/nan/ with a scripted player that checks in every
// `--every` seconds, and prints when each feature opens, each boss falls and each rebirth.
// tools/sim.sh [--hours 48] [--every 60] [--seed 1]
#include <math.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "../src/nan/Format.h"
#include "../src/nan/Game.h"

static uint32_t state = 1;
static uint32_t rng()
{
    state ^= state << 13;
    state ^= state >> 17;
    state ^= state << 5;
    return state;
}

static const char *FEATURE_NAMES[FEATURE_COUNT] = {"Boss", "XP", "Aventura", "Mineradores", "Aprimorar",
                                                   "Rebirth"};

static void stamp(double t)
{
    char buf[16];
    formatTime(t, buf, sizeof(buf));
    printf("%9s ", buf);
}

static void equipBest(Game &game)
{
    for (uint8_t place = 0; place < EQUIP_COUNT; place++)
    {
        int8_t best = game.equippedAt(place);
        double bestScore = best >= 0 ? game.itemPercent(best, true) + game.itemPercent(best, false) : 0;
        for (uint8_t i = 0; i < ITEM_COUNT; i++)
        {
            if (game.itemLevel(i) == 0 || ITEMS[i].slot != Game::placeSlot(place))
            {
                continue;
            }
            bool worn = false;
            for (uint8_t o = 0; o < EQUIP_COUNT; o++)
            {
                worn |= o != place && game.equippedAt(o) == i;
            }
            double score = game.itemPercent(i, true) + game.itemPercent(i, false);
            if (!worn && score > bestScore)
            {
                best = i;
                bestScore = score;
            }
        }
        if (best >= 0)
        {
            game.equip(place, best);
        }
    }
}

static void checkIn(Game &game)
{
    // Energy: everything idle goes to training, a third to augments once they open
    game.reclaimAll();
    double idle = game.idleEnergy();
    uint8_t open = 0;
    for (uint8_t i = 0; i < SKILL_COUNT; i++)
    {
        open += game.skillOpen(i);
    }
    double toAugments = game.isUnlocked(FEAT_AUGMENTS) ? idle / 3 : 0;
    for (uint8_t i = 0; i < AUGMENT_COUNT; i++)
    {
        game.allocate({true, i}, floor(toAugments / AUGMENT_COUNT));
    }
    idle = game.idleEnergy();
    for (uint8_t i = 0; i < SKILL_COUNT; i++)
    {
        if (game.skillOpen(i))
        {
            game.allocate({false, i}, floor(idle / open));
        }
    }

    bool bought = true;
    while (bought && game.isUnlocked(FEAT_XP))
    {
        bought = false;
        XpUpgrade cheapest = XpUpgrade::CAP;
        for (uint8_t u = 1; u < (uint8_t)XpUpgrade::COUNT; u++)
        {
            if (game.xpCost((XpUpgrade)u) < game.xpCost(cheapest))
            {
                cheapest = (XpUpgrade)u;
            }
        }
        bought = game.buyXp(cheapest);
    }
    bought = true;
    while (bought && game.isUnlocked(FEAT_MINERS))
    {
        bought = false;
        uint8_t best = 0;
        for (uint8_t i = 1; i < MINER_COUNT; i++)
        {
            if (game.minerCost(i) / MINERS[i].goldPerSecond < game.minerCost(best) / MINERS[best].goldPerSecond)
            {
                best = i;
            }
        }
        // Keep some gold for the augments
        if (game.minerCost(best) < game.gold() / 2)
        {
            bought = game.buyMiner(best);
        }
    }
    equipBest(game);

    if (!game.isFighting() && game.isUnlocked(FEAT_BOSS) && game.bossPlayerHp() >= game.maxHp() * 0.99 &&
        game.bossDps() > 0)
    {
        game.setFighting(true);
    }

    if (game.isUnlocked(FEAT_ADVENTURE) && game.advHp() >= game.maxHp() * 0.99)
    {
        uint8_t target = 0;
        for (uint8_t z = 1; z <= ZONE_COUNT; z++)
        {
            if (!game.zoneOpen(z))
            {
                continue;
            }
            game.setZone(z);
            // Somewhere it can kill and is not killed within ten hits
            double atk = game.attack(), def = game.defense();
            double eDef = Game::bossDefense(ZONES[z - 1].refBoss) * ZONES[z - 1].enemies[3].defense;
            double eAtk = Game::bossAttack(ZONES[z - 1].refBoss) * ZONES[z - 1].enemies[3].attack;
            if (atk - eDef / 2 > 0 && (eAtk - def / 2) * 10 < game.maxHp())
            {
                target = z;
            }
        }
        game.setZone(target);
    }
}

int main(int argc, char **argv)
{
    double hours = 48;
    double every = 60;
    for (int i = 1; i + 1 < argc; i += 2)
    {
        if (!strcmp(argv[i], "--hours")) hours = atof(argv[i + 1]);
        if (!strcmp(argv[i], "--every")) every = atof(argv[i + 1]);
        if (!strcmp(argv[i], "--seed")) state = (uint32_t)atoi(argv[i + 1]) | 1;
    }

    Game game(rng);
    uint16_t lastBeaten = 0;
    uint32_t lastUnlocks = 0;
    uint8_t lastZone = 0;
    uint32_t lastDeaths = 0;
    double lastBossAt = 0;
    const double dt = 0.1;
    double nextCheck = 0;
    for (double t = 0; t < hours * 3600; t += dt)
    {
        if (t >= nextCheck)
        {
            checkIn(game);
            nextCheck = t + every;
            if (game.canRebirth() && t - lastBossAt > 2 * 3600)
            {
                stamp(t);
                printf("REBIRTH  +%u NaN (boss %u, run %us)\n", game.rebirthPoints(), game.bossesBeaten(),
                       game.runSeconds());
                game.rebirth();
                lastBeaten = 0;
                lastBossAt = t;
                checkIn(game);
            }
        }
        game.tick(dt);
        if (game.unlockEvents() != lastUnlocks)
        {
            lastUnlocks = game.unlockEvents();
            stamp(t);
            printf("UNLOCK   %s\n", FEATURE_NAMES[game.lastUnlock()]);
        }
        if (game.bossesBeaten() != lastBeaten)
        {
            lastBeaten = game.bossesBeaten();
            lastBossAt = t;
            char a[12], d[12];
            formatNumber(game.attack(), a, sizeof(a));
            formatNumber(game.defense(), d, sizeof(d));
            stamp(t);
            printf("BOSS %2u  %-15s atk %s def %s  xp-bought %u/%u/%u/%u\n", lastBeaten,
                   Game::bossDef(lastBeaten - 1).name, a, d, game.xpBought(XpUpgrade::CAP),
                   game.xpBought(XpUpgrade::REGEN), game.xpBought(XpUpgrade::POWER), game.xpBought(XpUpgrade::LUCK));
        }
        if (game.zone() != lastZone && game.zone() != 0)
        {
            stamp(t);
            printf("ZONE     %s\n", ZONES[game.zone() - 1].name);
        }
        lastZone = game.zone() != 0 ? game.zone() : lastZone;
        if (game.advDeaths() != lastDeaths && game.advDeaths() % 50 == 1)
        {
            stamp(t);
            printf("DEATHS   %u\n", game.advDeaths());
        }
        lastDeaths = game.advDeaths();
    }
    char g[12];
    formatNumber(game.gold(), g, sizeof(g));
    printf("end: boss %u, kills %u, deaths %u, gold %s, items", game.bossesBeaten(), game.advKills(),
           game.advDeaths(), g);
    for (uint8_t i = 0; i < ITEM_COUNT; i++)
    {
        printf(" %u", game.itemLevel(i));
    }
    printf("\naugments %u/%u/%u, miners %u/%u/%u/%u/%u, NaN %u\n", game.augmentLevel(0), game.augmentLevel(1),
           game.augmentLevel(2), game.minerCount(0), game.minerCount(1), game.minerCount(2), game.minerCount(3),
           game.minerCount(4), game.nan());
}
