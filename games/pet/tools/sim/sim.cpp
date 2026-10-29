// Balance simulator: runs the real World and Colony on a simulated clock with a scripted
// player (active for the first half hour, then a short check-in every waking hour, asleep
// 00:00-08:00), and prints when each purchase first happens and how the colony grows.
#include <Arduino.h>
#include <random>
#include <vector>
#include "../../src/game/World.h"
#include "../../src/game/Colony.h"

static unsigned long simNow = 0;
static std::mt19937 rng(1);

unsigned long millis() { return simNow; }
long random(long max) { return max <= 0 ? 0 : (long)(rng() % (unsigned long)max); }
long random(long min, long max) { return min + random(max - min); }

struct Options
{
    unsigned days = 2;
    unsigned seed = 1;
    unsigned sessionMinutes = 5;
    bool log = false;
};

static const unsigned long MINUTE = 60000;
static const unsigned long HOUR = 60 * MINUTE;
static const unsigned long ACTIVE_START = 30 * MINUTE;
static const unsigned long ACT_EVERY_MS = 5000;
static const unsigned START_HOUR = 8;

static void stamp(char *out, size_t size, unsigned long t)
{
    unsigned long minutes = t / MINUTE;
    snprintf(out, size, "d%lu %02lu:%02lu", minutes / (24 * 60) + 1, (minutes / 60 + START_HOUR) % 24, minutes % 60);
}

static bool awake(unsigned long t)
{
    unsigned hour = (t / HOUR + START_HOUR) % 24;
    return hour >= 8;
}

static bool playing(unsigned long t, const Options &o)
{
    if (t < ACTIVE_START)
    {
        return true;
    }
    return awake(t) && t % HOUR < o.sessionMinutes * MINUTE;
}

class Player
{
public:
    Player(World &world, Colony &colony, const Options &o) : world(world), colony(colony), o(o) {}

    void act()
    {
        tendCrops();
        buy();
    }

    void report(unsigned long t)
    {
        char when[24];
        stamp(when, sizeof(when), t);
        printf("%-10s pop %2u/%-2u  coins %8llu  +%5llu/min  food %4u/%-4u  soil %2u  ponds %u  flowers %2u  left %u\n",
               when, colony.getCount(), colony.getCapacity(), (unsigned long long)world.getCoins(),
               (unsigned long long)colony.getTotalCoinsPerMinute(), world.getFood(), world.getPantrySize(),
               world.count(TileKind::SOIL) + world.count(TileKind::CROP), world.count(TileKind::POND),
               world.count(TileKind::FLOWERS), (unsigned)colony.getDepartures());
    }

    void noteEvents(unsigned long t)
    {
        if (o.log && colony.getEventCount() != seenEvents && colony.getLastEvent() == ColonyEvent::LEFT)
        {
            log(t, "LEFT", SPECIES[colony.getLastEventSpecies()].name);
        }
        seenEvents = colony.getEventCount();
    }

private:
    World &world;
    Colony &colony;
    const Options &o;
    uint32_t seenEvents = 0;
    bool firstAdopt[SPECIES_COUNT] = {};
    bool mapCleared = false;
    unsigned long now = 0;

    void log(unsigned long t, const char *what, const char *name)
    {
        char when[24];
        stamp(when, sizeof(when), t);
        printf("%-10s %-8s %s\n", when, what, name);
    }

    void tendCrops()
    {
        uint8_t best = 0;
        for (uint8_t c = 0; c < CROP_COUNT; c++)
        {
            if (world.isCropUnlocked(c))
            {
                best = c;
            }
        }
        for (uint16_t i = 0; i < TILE_COUNT; i++)
        {
            if (world.isRipe(i))
            {
                world.harvest(i);
            }
            else if (world.kindAt(i) == TileKind::CROP && !world.isWatered(i))
            {
                world.water(i);
            }
            if (world.kindAt(i) == TileKind::SOIL)
            {
                // The best crop that still ripens before the next check-in, while active
                uint8_t c = best;
                while (c > 0 && (!world.isCropUnlocked(c) || world.getCoins() < CROPS[c].seedPrice * 4))
                {
                    c--;
                }
                if (world.plant(i, c))
                {
                    world.water(i);
                }
            }
        }
    }

    int16_t findGrass(TileKind near, uint8_t range) const
    {
        int16_t fallback = -1;
        for (uint16_t i = 0; i < TILE_COUNT; i++)
        {
            if (world.kindAt(i) != TileKind::GRASS)
            {
                continue;
            }
            if (fallback < 0)
            {
                fallback = i;
            }
            if (world.countNear(near, i % GRID_COLS, i / GRID_COLS, range) > 0)
            {
                return i;
            }
        }
        return fallback;
    }

    bool place(BuildId b, TileKind near, uint8_t range)
    {
        int16_t i = findGrass(near, range);
        if (i < 0)
        {
            clearOne();
            return false;
        }
        if (!world.build(i, b))
        {
            return false;
        }
        if (o.log)
        {
            log(now, "BUILD", BUILDS[b].name);
        }
        return true;
    }

    void clearOne()
    {
        for (uint16_t i = 0; i < TILE_COUNT; i++)
        {
            if (world.canClear(i))
            {
                world.clear(i);
                return;
            }
        }
    }

    // Food per minute the colony eats, against what its fields give between check-ins:
    // a crop away from a pond grows only for the hand-watering it gets
    bool shortOfFood() const
    {
        double eats = 0;
        for (uint8_t i = 0; i < colony.getCount(); i++)
        {
            const SpeciesDef &s = SPECIES[colony.get(i).species];
            eats += (double)s.mealFood / s.mealMinutes;
        }
        double grows = 0;
        uint8_t range = POND_RANGE + world.getEffect(UpgradeEffect::POND_RANGE);
        for (uint16_t i = 0; i < TILE_COUNT; i++)
        {
            if (world.kindAt(i) == TileKind::CROP)
            {
                const CropDef &c = CROPS[world.cropAt(i)];
                double perMinute = world.harvestFood(world.cropAt(i)) * 60.0 * 3 / (2 * c.growSeconds);
                bool pond = world.countNear(TileKind::POND, i % GRID_COLS, i / GRID_COLS, range) > 0;
                grows += pond ? perMinute : perMinute / 8;
            }
        }
        return grows < eats * 1.5;
    }

    bool adoptBest()
    {
        for (int8_t s = SPECIES_COUNT - 1; s >= 0; s--)
        {
            if (colony.adopt(s))
            {
                if (!firstAdopt[s])
                {
                    firstAdopt[s] = true;
                    log(now, "ADOPT", SPECIES[s].name);
                }
                return true;
            }
        }
        return false;
    }

    void buy()
    {
        if (world.count(TileKind::GRASS) < 2)
        {
            clearOne();
        }
        if (shortOfFood())
        {
            uint16_t fields = world.count(TileKind::SOIL) + world.count(TileKind::CROP);
            if (fields >= 4 && world.count(TileKind::POND) * 8 < fields)
            {
                place(BUILD_POND, TileKind::CROP, 1);
                return;
            }
            place(BUILD_SOIL, TileKind::POND, POND_RANGE);
            return;
        }
        if (world.getFood() * 10 > world.getPantrySize() * 8 && world.count(TileKind::TROUGH) < 1 + colony.getCount() / 4)
        {
            place(BUILD_TROUGH, TileKind::BURROW, 3);
            return;
        }
        if (world.count(TileKind::FLOWERS) < world.count(TileKind::BURROW) * 2)
        {
            place(BUILD_FLOWERS, TileKind::BURROW, 1);
            return;
        }
        if (colony.getCount() + 1 >= colony.getCapacity() && colony.getCapacity() < MAX_CRITTERS)
        {
            place(BUILD_BURROW, TileKind::FLOWERS, FLOWER_RANGE);
            return;
        }
        if (adoptBest() || swapWorst())
        {
            return;
        }
        for (uint8_t c = 1; c < CROP_COUNT; c++)
        {
            if (!world.isCropUnlocked(c) && world.unlockCrop(c))
            {
                log(now, "SEEDS", CROPS[c].name);
                return;
            }
        }
        for (uint8_t u = 0; u < UPGRADE_COUNT; u++)
        {
            if (!world.hasUpgrade(u) && world.buyUpgrade(u, colony.getCount()))
            {
                log(now, "UPGRADE", UPGRADES[u].name);
                return;
            }
        }
        // Nothing else to want: push the wild back, keeping enough for the next species
        uint8_t next = 0;
        while (next < SPECIES_COUNT && colony.countOf(next) > 0)
        {
            next++;
        }
        uint64_t reserve = next < SPECIES_COUNT ? colony.adoptPrice(next) / 2 : 0;
        if (world.getCoins() > reserve + world.clearPrice())
        {
            clearOne();
            if (world.count(TileKind::WILD) == 0 && !mapCleared)
            {
                mapCleared = true;
                log(now, "CLEARED", "the whole map");
            }
        }
    }

    // A full colony makes room for a better species by letting its weakest one go
    bool swapWorst()
    {
        if (colony.getCount() < colony.getCapacity() || colony.getCount() == 0)
        {
            return false;
        }
        uint8_t worst = 0;
        for (uint8_t i = 1; i < colony.getCount(); i++)
        {
            if (colony.get(i).species < colony.get(worst).species)
            {
                worst = i;
            }
        }
        uint8_t from = colony.get(worst).species;
        for (int8_t s = SPECIES_COUNT - 1; s > from; s--)
        {
            if (colony.getCount() >= SPECIES[s].reqPopulation && world.getCoins() >= colony.adoptPrice(s))
            {
                colony.release(worst);
                return adoptBest();
            }
        }
        return false;
    }

public:
    void setNow(unsigned long t) { now = t; }
};

int main(int argc, char **argv)
{
    Options o;
    for (int i = 1; i < argc; i++)
    {
        std::string arg = argv[i];
        auto next = [&]() { return i + 1 < argc ? (unsigned)atoi(argv[++i]) : 0u; };
        if (arg == "--days")
        {
            o.days = next();
        }
        else if (arg == "--seed")
        {
            o.seed = next();
        }
        else if (arg == "--session")
        {
            o.sessionMinutes = next();
        }
        else if (arg == "--log")
        {
            o.log = true;
        }
    }
    rng.seed(o.seed);

    World world;
    Colony colony(world);
    Player player(world, colony, o);

    unsigned long end = o.days * 24 * HOUR;
    unsigned long nextAct = 0;
    for (simNow = TICK_MS; simNow < end; simNow += TICK_MS)
    {
        world.update(simNow);
        colony.update(simNow);
        player.setNow(simNow);
        player.noteEvents(simNow);
        if (playing(simNow, o) && (long)(simNow - nextAct) >= 0)
        {
            nextAct = simNow + ACT_EVERY_MS;
            player.act();
        }
        if (simNow % HOUR == 0 && (awake(simNow) || simNow % (4 * HOUR) == 0))
        {
            player.report(simNow);
        }
    }
    printf("births %u, departures %u\n", (unsigned)colony.getBirths(), (unsigned)colony.getDepartures());
    return 0;
}
