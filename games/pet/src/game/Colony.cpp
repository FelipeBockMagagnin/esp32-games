#include "Colony.h"

static const uint8_t START_CRITTERS = 2;
static const uint8_t WANDER_STEP_CHANCE = 35; // Percent of ticks an idle critter takes a step

Colony::Colony(World &world)
    : world(world), count(0), nextName(0), breedTicks(0), nextTick(0), eventCount(0), lastEvent(ColonyEvent::NONE),
      lastEventName(0), lastEventSpecies(0), births(0), departures(0), revision(0)
{
    reset();
}

void Colony::reset()
{
    memset(critters, 0, sizeof(critters));
    count = 0;
    nextName = 0;
    breedTicks = 0;
    for (uint8_t i = 0; i < START_CRITTERS; i++)
    {
        spawn(0);
    }
}

void Colony::update(unsigned long now)
{
    if (nextTick != 0 && (long)(now - nextTick) < 0)
    {
        return;
    }
    nextTick = now + TICK_MS;
    tick();
}

void Colony::tick()
{
    // Backwards, so a critter that leaves does not skip the one moved into its place
    for (int16_t i = count - 1; i >= 0; i--)
    {
        tickCritter(critters[i], i);
    }
    if (++breedTicks >= BREED_SECONDS * TICKS_PER_SECOND)
    {
        breedTicks = 0;
        tryBreed();
    }
}

void Colony::tickCritter(Critter &c, uint8_t index)
{
    const SpeciesDef &def = SPECIES[c.species];
    bool asleep = c.state == (uint8_t)CritterState::SLEEPING;

    uint32_t hungerStep = HUNGRY_AT / ((uint32_t)def.mealMinutes * 60 * TICKS_PER_SECOND);
    uint32_t hunger = c.hunger + (asleep ? hungerStep / 2 : hungerStep);
    c.hunger = hunger > NEED_MAX ? NEED_MAX : hunger;
    if (c.hunger >= NEED_MAX)
    {
        if (++c.starving >= LEAVE_SECONDS * TICKS_PER_SECOND)
        {
            record(ColonyEvent::LEFT, c);
            remove(index);
            return;
        }
    }
    else
    {
        c.starving = 0;
    }

    if (!asleep)
    {
        uint32_t drain = NEED_MAX / ((uint32_t)AWAKE_SECONDS * TICKS_PER_SECOND);
        c.energy = c.energy > drain ? c.energy - drain : 0;
        world.creditCoins((uint64_t)def.coinsPerMinute * 1000 * getMood(index) / 100 / (60 * TICKS_PER_SECOND));
    }

    uint16_t here = World::indexOf(c.x, c.y);
    uint8_t stepX, stepY;
    switch ((CritterState)c.state)
    {
    case CritterState::WANDER:
        if (c.energy < TIRED_AT)
        {
            bool bed = world.findPath(TileKind::BURROW, c.x, c.y, stepX, stepY);
            c.state = (uint8_t)(bed ? CritterState::TO_BED : CritterState::SLEEPING);
            world.markChanged(here);
        }
        else if (c.hunger >= HUNGRY_AT && world.getFood() >= def.mealFood &&
                 world.findPath(TileKind::TROUGH, c.x, c.y, stepX, stepY))
        {
            c.state = (uint8_t)CritterState::TO_FOOD;
        }
        else if (world.getFood() < world.getPantrySize() &&
                 world.findPath(TileKind::CROP, c.x, c.y, stepX, stepY, true))
        {
            c.state = (uint8_t)CritterState::TO_CROP;
        }
        else
        {
            wander(c);
        }
        break;

    case CritterState::TO_CROP:
        if (!world.findPath(TileKind::CROP, c.x, c.y, stepX, stepY, true))
        {
            c.state = (uint8_t)CritterState::WANDER;
        }
        else if (stepX != c.x || stepY != c.y)
        {
            moveTo(c, stepX, stepY);
        }
        else
        {
            // Another got there first, or the pantry filled up on the way: either way it is done
            world.harvest(here);
            c.state = (uint8_t)CritterState::WANDER;
        }
        break;

    case CritterState::TO_FOOD:
    case CritterState::TO_BED:
    {
        bool toFood = c.state == (uint8_t)CritterState::TO_FOOD;
        // The way can close behind it, or the place be cleared away
        if (!world.findPath(toFood ? TileKind::TROUGH : TileKind::BURROW, c.x, c.y, stepX, stepY))
        {
            c.state = (uint8_t)(toFood ? CritterState::WANDER : CritterState::SLEEPING);
            world.markChanged(here);
            break;
        }
        if (stepX != c.x || stepY != c.y)
        {
            moveTo(c, stepX, stepY);
            break;
        }
        if (!toFood)
        {
            c.state = (uint8_t)CritterState::SLEEPING;
        }
        else if (world.takeFood(def.mealFood))
        {
            c.state = (uint8_t)CritterState::EATING;
            c.timer = EAT_TICKS;
            c.hunger = 0;
        }
        else
        {
            c.state = (uint8_t)CritterState::WANDER;
        }
        world.markChanged(here);
        break;
    }

    case CritterState::EATING:
        if (c.timer > 0)
        {
            c.timer--;
        }
        else
        {
            c.state = (uint8_t)CritterState::WANDER;
            world.markChanged(here);
        }
        break;

    case CritterState::SLEEPING:
    {
        bool inBurrow = world.kindAt(here) == TileKind::BURROW;
        uint32_t rest = NEED_MAX / ((uint32_t)SLEEP_SECONDS * TICKS_PER_SECOND);
        uint32_t energy = c.energy + (inBurrow ? rest : rest / 2);
        c.energy = energy > NEED_MAX ? NEED_MAX : energy;
        if (c.energy >= NEED_MAX)
        {
            c.state = (uint8_t)CritterState::WANDER;
            world.markChanged(here);
        }
        break;
    }

    default:
        c.state = (uint8_t)CritterState::WANDER;
        break;
    }
}

void Colony::moveTo(Critter &c, uint8_t x, uint8_t y)
{
    world.markChanged(World::indexOf(c.x, c.y));
    c.x = x;
    c.y = y;
    world.markChanged(World::indexOf(x, y));
}

const char *describeCritter(const Critter &c)
{
    static const char *const NAMES[(uint8_t)CritterState::COUNT] = {
        "wandering", "going to eat", "eating", "going to bed", "sleeping", "gathering"};
    if (c.hunger >= NEED_MAX)
    {
        return "starving!";
    }
    if (c.hunger >= HUNGRY_AT && c.state == (uint8_t)CritterState::WANDER)
    {
        return "hungry";
    }
    return c.state < (uint8_t)CritterState::COUNT ? NAMES[c.state] : "";
}

void Colony::wander(Critter &c)
{
    if (random(100) >= WANDER_STEP_CHANCE)
    {
        return;
    }
    static const int8_t DX[] = {1, -1, 0, 0}, DY[] = {0, 0, 1, -1};
    uint8_t d = random(4);
    int16_t nx = c.x + DX[d], ny = c.y + DY[d];
    if (world.isWalkable(nx, ny))
    {
        moveTo(c, nx, ny);
    }
}

uint8_t Colony::countOf(uint8_t species) const
{
    uint8_t n = 0;
    for (uint8_t i = 0; i < count; i++)
    {
        n += critters[i].species == species;
    }
    return n;
}

uint8_t Colony::getCapacity() const
{
    uint32_t room = (uint32_t)world.count(TileKind::BURROW) * (BURROW_ROOM + world.getEffect(UpgradeEffect::BURROW_ROOM));
    return room < MAX_CRITTERS ? room : MAX_CRITTERS;
}

uint8_t Colony::getMood(uint8_t i) const
{
    const Critter &c = critters[i];
    uint32_t flowers = world.countNear(TileKind::FLOWERS, c.x, c.y, FLOWER_RANGE);
    uint32_t cheer = flowers * (MOOD_PER_FLOWER + world.getEffect(UpgradeEffect::FLOWER_MOOD));
    int16_t mood = MOOD_BASE + (cheer < MOOD_FLOWER_CAP ? cheer : MOOD_FLOWER_CAP);
    mood += isHoused() ? MOOD_HOUSED : -(int16_t)MOOD_HOUSED;
    if (c.hunger >= NEED_MAX)
    {
        mood -= MOOD_STARVING;
    }
    else if (c.hunger >= HUNGRY_AT)
    {
        mood -= MOOD_HUNGRY;
    }
    return mood < 0 ? 0 : (mood > 100 ? 100 : mood);
}

uint64_t Colony::getCoinsPerMinute(uint8_t i) const
{
    return (uint64_t)SPECIES[critters[i].species].coinsPerMinute * getMood(i) / 100;
}

uint64_t Colony::getTotalCoinsPerMinute() const
{
    uint64_t total = 0;
    for (uint8_t i = 0; i < count; i++)
    {
        total += getCoinsPerMinute(i);
    }
    return total;
}

uint32_t Colony::adoptPrice(uint8_t species) const
{
    return SPECIES[species].price + SPECIES[species].price / 4 * countOf(species);
}

bool Colony::canAdopt(uint8_t species) const
{
    return species < SPECIES_COUNT && count >= SPECIES[species].reqPopulation && count < getCapacity() &&
           world.getCoins() >= adoptPrice(species);
}

bool Colony::adopt(uint8_t species)
{
    if (!canAdopt(species) || !world.spend(adoptPrice(species)))
    {
        return false;
    }
    spawn(species);
    revision++;
    return true;
}

void Colony::release(uint8_t i)
{
    if (i < count)
    {
        remove(i);
    }
}

// At a burrow when there is one, else anywhere open
bool Colony::spawn(uint8_t species)
{
    if (count >= MAX_CRITTERS)
    {
        return false;
    }
    uint16_t open[TILE_COUNT];
    uint16_t openCount = 0;
    for (TileKind k : {TileKind::BURROW, TileKind::GRASS})
    {
        for (uint16_t i = 0; i < TILE_COUNT; i++)
        {
            if (world.kindAt(i) == k)
            {
                open[openCount++] = i;
            }
        }
        if (openCount > 0)
        {
            break;
        }
    }
    if (openCount == 0)
    {
        return false;
    }
    uint16_t at = open[random(openCount)];
    Critter &c = critters[count++];
    c = Critter();
    c.species = species;
    c.name = nextName;
    nextName = (nextName + 1) % CRITTER_NAME_COUNT;
    c.x = at % GRID_COLS;
    c.y = at / GRID_COLS;
    c.state = (uint8_t)CritterState::WANDER;
    c.energy = NEED_MAX;
    world.markChanged(at);
    return true;
}

void Colony::remove(uint8_t index)
{
    world.markChanged(World::indexOf(critters[index].x, critters[index].y));
    critters[index] = critters[count - 1];
    count--;
    revision++;
}

void Colony::tryBreed()
{
    if (count >= getCapacity() || count >= MAX_CRITTERS)
    {
        return;
    }
    uint8_t first = random(SPECIES_COUNT);
    for (uint8_t step = 0; step < SPECIES_COUNT; step++)
    {
        uint8_t s = (first + step) % SPECIES_COUNT;
        uint8_t members = 0;
        uint32_t moodSum = 0;
        for (uint8_t i = 0; i < count; i++)
        {
            if (critters[i].species == s)
            {
                members++;
                moodSum += getMood(i);
            }
        }
        if (members < 2 || moodSum / members < BREED_MOOD)
        {
            continue;
        }
        if (random(100) < BREED_CHANCE_PERCENT && spawn(s))
        {
            revision++;
            record(ColonyEvent::BORN, critters[count - 1]);
        }
        return;
    }
}

void Colony::record(ColonyEvent event, const Critter &c)
{
    lastEvent = event;
    lastEventName = c.name;
    lastEventSpecies = c.species;
    eventCount++;
    (event == ColonyEvent::BORN ? births : departures)++;
}

void Colony::save(Snapshot &out) const
{
    memcpy(out.critters, critters, sizeof(critters));
    out.breedTicks = breedTicks;
    out.count = count;
    out.nextName = nextName;
}

void Colony::load(const Snapshot &in)
{
    // A zeroed snapshot is a new game
    if (in.count == 0 && in.nextName == 0)
    {
        reset();
        return;
    }
    memcpy(critters, in.critters, sizeof(critters));
    count = in.count <= MAX_CRITTERS ? in.count : MAX_CRITTERS;
    nextName = in.nextName % CRITTER_NAME_COUNT;
    breedTicks = in.breedTicks;
    for (uint8_t i = 0; i < count; i++)
    {
        Critter &c = critters[i];
        if (c.species >= SPECIES_COUNT || c.x >= GRID_COLS || c.y >= GRID_ROWS || c.name >= CRITTER_NAME_COUNT)
        {
            c = Critter();
            c.x = START_LEFT;
            c.y = START_TOP;
            c.energy = NEED_MAX;
        }
        if (c.state >= (uint8_t)CritterState::COUNT)
        {
            c.state = (uint8_t)CritterState::WANDER;
        }
    }
}
