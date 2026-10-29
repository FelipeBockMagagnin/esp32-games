#include "World.h"
#include "SatMath.h"

World::World()
    : coins(0), coinFraction(0), food(0), cleared(0), upgradeFlags(0), cropFlags(0), revision(0), nextTick(0),
      tickInSecond(0)
{
    reset();
}

void World::reset()
{
    memset(kind, (uint8_t)TileKind::WILD, sizeof(kind));
    memset(crop, 0, sizeof(crop));
    memset(waterLeft, 0, sizeof(waterLeft));
    memset(growth, 0, sizeof(growth));
    for (uint8_t y = START_TOP; y < START_TOP + START_SIZE; y++)
    {
        for (uint8_t x = START_LEFT; x < START_LEFT + START_SIZE; x++)
        {
            kind[indexOf(x, y)] = (uint8_t)TileKind::GRASS;
        }
    }
    kind[indexOf(START_LEFT, START_TOP)] = (uint8_t)TileKind::BURROW;
    kind[indexOf(START_LEFT + START_SIZE - 1, START_TOP)] = (uint8_t)TileKind::TROUGH;
    for (uint8_t x = START_LEFT + 1; x < START_LEFT + START_SIZE - 1; x++)
    {
        kind[indexOf(x, START_TOP + START_SIZE - 1)] = (uint8_t)TileKind::SOIL;
    }
    coins = START_COINS;
    coinFraction = 0;
    food = START_FOOD;
    cleared = 0;
    upgradeFlags = 0;
    cropFlags = 0;
    markAllChanged();
}

void World::update(unsigned long now)
{
    if (nextTick != 0 && (long)(now - nextTick) < 0)
    {
        return;
    }
    nextTick = now + TICK_MS;
    tick();
}

void World::tick()
{
    bool secondDone = ++tickInSecond >= TICKS_PER_SECOND;
    if (secondDone)
    {
        tickInSecond = 0;
    }
    uint32_t speed = 100 + getEffect(UpgradeEffect::GROW_PERCENT);
    for (uint16_t i = 0; i < TILE_COUNT; i++)
    {
        if (kind[i] != (uint8_t)TileKind::CROP || growth[i] >= GROWTH_FULL)
        {
            continue;
        }
        bool wet = isWatered(i);
        uint8_t stage = cropStage(i);
        if (wet)
        {
            uint32_t step = (uint32_t)GROWTH_FULL * speed / (100u * CROPS[crop[i]].growSeconds * TICKS_PER_SECOND);
            uint32_t grown = growth[i] + (step > 0 ? step : 1);
            growth[i] = grown > GROWTH_FULL ? GROWTH_FULL : grown;
        }
        if (secondDone && waterLeft[i] > 0)
        {
            waterLeft[i]--;
        }
        if (cropStage(i) != stage || isWatered(i) != wet)
        {
            markChanged(i);
        }
    }
}

bool World::isWalkable(int16_t x, int16_t y) const
{
    if (!inGrid(x, y))
    {
        return false;
    }
    TileKind k = (TileKind)kind[indexOf(x, y)];
    return k != TileKind::WILD && k != TileKind::POND;
}

uint8_t World::cropStage(uint16_t i) const
{
    if (growth[i] >= GROWTH_FULL)
    {
        return CROP_STAGES - 1;
    }
    return (uint32_t)growth[i] * (CROP_STAGES - 1) / GROWTH_FULL;
}

bool World::nearPond(uint16_t i) const
{
    return countNear(TileKind::POND, i % GRID_COLS, i / GRID_COLS, POND_RANGE + getEffect(UpgradeEffect::POND_RANGE)) > 0;
}

bool World::isWatered(uint16_t i) const
{
    return waterLeft[i] > 0 || nearPond(i);
}

uint16_t World::count(TileKind k) const
{
    uint16_t n = 0;
    for (uint16_t i = 0; i < TILE_COUNT; i++)
    {
        n += kind[i] == (uint8_t)k;
    }
    return n;
}

bool World::findPath(TileKind k, uint8_t x, uint8_t y, uint8_t &stepX, uint8_t &stepY, bool ripe) const
{
    static const int8_t DX[] = {1, -1, 0, 0}, DY[] = {0, 0, 1, -1};
    static const uint16_t UNSEEN = 0xFFFF;
    // Static: the loop task's stack is small, and only one search runs at a time
    static uint16_t cameFrom[TILE_COUNT];
    static uint16_t queue[TILE_COUNT];

    auto isGoal = [&](uint16_t i) { return kind[i] == (uint8_t)k && (!ripe || isRipe(i)); };
    uint16_t start = indexOf(x, y);
    if (isGoal(start))
    {
        stepX = x;
        stepY = y;
        return true;
    }
    for (uint16_t &from : cameFrom)
    {
        from = UNSEEN;
    }
    cameFrom[start] = start;
    uint16_t head = 0, tail = 0;
    queue[tail++] = start;
    while (head < tail)
    {
        uint16_t at = queue[head++];
        int16_t ax = at % GRID_COLS, ay = at / GRID_COLS;
        for (uint8_t d = 0; d < 4; d++)
        {
            int16_t nx = ax + DX[d], ny = ay + DY[d];
            if (!inGrid(nx, ny))
            {
                continue;
            }
            uint16_t next = indexOf(nx, ny);
            if (cameFrom[next] != UNSEEN)
            {
                continue;
            }
            bool goal = isGoal(next);
            if (!goal && !isWalkable(nx, ny))
            {
                continue;
            }
            cameFrom[next] = at;
            if (goal)
            {
                while (cameFrom[next] != start)
                {
                    next = cameFrom[next];
                }
                stepX = next % GRID_COLS;
                stepY = next / GRID_COLS;
                return true;
            }
            queue[tail++] = next;
        }
    }
    return false;
}

uint8_t World::countNear(TileKind k, uint8_t x, uint8_t y, uint8_t range) const
{
    uint8_t n = 0;
    for (int16_t ty = y - range; ty <= y + range; ty++)
    {
        for (int16_t tx = x - range; tx <= x + range; tx++)
        {
            if (inGrid(tx, ty) && kind[indexOf(tx, ty)] == (uint8_t)k)
            {
                n++;
            }
        }
    }
    return n;
}

bool World::canClear(uint16_t i) const
{
    if (kind[i] != (uint8_t)TileKind::WILD)
    {
        return false;
    }
    int16_t x = i % GRID_COLS, y = i / GRID_COLS;
    static const int8_t DX[] = {1, -1, 0, 0}, DY[] = {0, 0, 1, -1};
    for (uint8_t d = 0; d < 4; d++)
    {
        if (inGrid(x + DX[d], y + DY[d]) && kind[indexOf(x + DX[d], y + DY[d])] != (uint8_t)TileKind::WILD)
        {
            return true;
        }
    }
    return false;
}

uint32_t World::clearPrice() const
{
    uint64_t n = cleared;
    uint64_t price = CLEAR_BASE + n * n * n / CLEAR_DIVISOR;
    return price < UINT32_MAX ? price : UINT32_MAX;
}

bool World::clear(uint16_t i)
{
    if (!canClear(i) || !spend(clearPrice()))
    {
        return false;
    }
    kind[i] = (uint8_t)TileKind::GRASS;
    cleared++;
    markChanged(i);
    return true;
}

uint32_t World::buildPrice(BuildId b) const
{
    const BuildDef &def = BUILDS[b];
    uint64_t price = def.price;
    for (uint16_t n = count(def.kind); n > 0 && price < UINT32_MAX; n--)
    {
        price = price * (100 + def.growthPercent) / 100;
    }
    return price < UINT32_MAX ? price : UINT32_MAX;
}

bool World::build(uint16_t i, BuildId b)
{
    if (kind[i] != (uint8_t)TileKind::GRASS || !spend(buildPrice(b)))
    {
        return false;
    }
    kind[i] = (uint8_t)BUILDS[b].kind;
    markPondArea(i);
    return true;
}

bool World::plant(uint16_t i, uint8_t c)
{
    if (kind[i] != (uint8_t)TileKind::SOIL || c >= CROP_COUNT || !isCropUnlocked(c) || !spend(CROPS[c].seedPrice))
    {
        return false;
    }
    kind[i] = (uint8_t)TileKind::CROP;
    crop[i] = c;
    growth[i] = 0;
    waterLeft[i] = 0;
    markChanged(i);
    return true;
}

bool World::water(uint16_t i)
{
    if (kind[i] != (uint8_t)TileKind::CROP || isRipe(i))
    {
        return false;
    }
    uint32_t seconds = (uint32_t)WATER_SECONDS * (100 + getEffect(UpgradeEffect::WATER_PERCENT)) / 100;
    uint8_t full = seconds > 255 ? 255 : seconds;
    if (waterLeft[i] == full)
    {
        return false;
    }
    waterLeft[i] = full;
    revision++;
    markChanged(i);
    return true;
}

uint32_t World::harvestFood(uint8_t c) const
{
    return CROPS[c].food * (100 + getEffect(UpgradeEffect::YIELD_PERCENT)) / 100;
}

bool World::harvest(uint16_t i)
{
    if (!isRipe(i))
    {
        return false;
    }
    uint32_t gain = harvestFood(crop[i]);
    if (food + gain > getPantrySize())
    {
        return false;
    }
    if (food == 0)
    {
        markKind(TileKind::TROUGH);
    }
    food += gain;
    growth[i] = GROWTH_FULL / (CROP_STAGES - 1);
    markChanged(i);
    return true;
}

bool World::demolish(uint16_t i)
{
    if (kind[i] == (uint8_t)TileKind::WILD || kind[i] == (uint8_t)TileKind::GRASS)
    {
        return false;
    }
    bool pond = kind[i] == (uint8_t)TileKind::POND;
    kind[i] = kind[i] == (uint8_t)TileKind::CROP ? (uint8_t)TileKind::SOIL : (uint8_t)TileKind::GRASS;
    growth[i] = 0;
    waterLeft[i] = 0;
    revision++;
    if (pond)
    {
        markPondArea(i);
    }
    markChanged(i);
    // A pantry that shrank keeps nothing past its new size
    if (food > getPantrySize())
    {
        food = getPantrySize();
    }
    return true;
}

bool World::spend(uint32_t price)
{
    if (coins < price)
    {
        return false;
    }
    coins -= price;
    revision++;
    return true;
}

void World::creditCoins(uint64_t thousandths)
{
    uint64_t total = satAdd(coinFraction, thousandths);
    coins = satAdd(coins, total / 1000);
    coinFraction = total % 1000;
}

uint32_t World::getPantrySize() const
{
    uint32_t perTrough = PANTRY_PER_TROUGH * (100 + getEffect(UpgradeEffect::PANTRY_PERCENT)) / 100;
    return PANTRY_BASE + perTrough * count(TileKind::TROUGH);
}

bool World::takeFood(uint32_t amount)
{
    if (food < amount)
    {
        return false;
    }
    food -= amount;
    if (food == 0)
    {
        markKind(TileKind::TROUGH);
    }
    return true;
}

bool World::unlockCrop(uint8_t c)
{
    if (c >= CROP_COUNT || isCropUnlocked(c) || !spend(CROPS[c].unlockPrice))
    {
        return false;
    }
    cropFlags |= 1u << c;
    return true;
}

bool World::buyUpgrade(uint8_t u, uint8_t population)
{
    if (u >= UPGRADE_COUNT || hasUpgrade(u) || population < UPGRADES[u].reqPopulation || !spend(UPGRADES[u].price))
    {
        return false;
    }
    upgradeFlags |= 1u << u;
    markAllChanged();
    return true;
}

uint16_t World::getEffect(UpgradeEffect effect) const
{
    uint16_t total = 0;
    for (uint8_t u = 0; u < UPGRADE_COUNT; u++)
    {
        if (hasUpgrade(u) && UPGRADES[u].effect == effect)
        {
            total += UPGRADES[u].value;
        }
    }
    return total;
}

// The crops a pond waters show it, so a pond coming or going repaints all it reaches
void World::markPondArea(uint16_t i)
{
    int16_t range = POND_RANGE + getEffect(UpgradeEffect::POND_RANGE);
    int16_t x = i % GRID_COLS, y = i / GRID_COLS;
    for (int16_t ty = y - range; ty <= y + range; ty++)
    {
        for (int16_t tx = x - range; tx <= x + range; tx++)
        {
            if (inGrid(tx, ty))
            {
                markChanged(indexOf(tx, ty));
            }
        }
    }
}

// Troughs show whether the pantry has anything in it
void World::markKind(TileKind k)
{
    for (uint16_t i = 0; i < TILE_COUNT; i++)
    {
        if (kind[i] == (uint8_t)k)
        {
            markChanged(i);
        }
    }
}

bool World::takeChanged(uint16_t i)
{
    uint8_t bit = 1 << (i % 8);
    if ((changed[i / 8] & bit) == 0)
    {
        return false;
    }
    changed[i / 8] &= ~bit;
    return true;
}

bool World::anyChanged() const
{
    for (uint8_t b : changed)
    {
        if (b != 0)
        {
            return true;
        }
    }
    return false;
}

void World::markAllChanged()
{
    memset(changed, 0xFF, sizeof(changed));
}

void World::save(Snapshot &out) const
{
    out.coins = coins;
    out.food = food;
    out.coinFraction = coinFraction;
    out.cleared = cleared;
    out.upgrades = upgradeFlags;
    out.crops = cropFlags;
    memcpy(out.kind, kind, sizeof(kind));
    memcpy(out.crop, crop, sizeof(crop));
    memcpy(out.water, waterLeft, sizeof(waterLeft));
    memcpy(out.growth, growth, sizeof(growth));
}

void World::load(const Snapshot &in)
{
    bool fresh = true;
    for (uint16_t i = 0; i < TILE_COUNT && fresh; i++)
    {
        fresh = in.kind[i] == (uint8_t)TileKind::WILD;
    }
    // A zeroed snapshot is all wild: a new game
    if (fresh)
    {
        reset();
        return;
    }
    coins = in.coins;
    food = in.food;
    coinFraction = in.coinFraction % 1000;
    cleared = in.cleared;
    upgradeFlags = in.upgrades;
    cropFlags = in.crops;
    memcpy(kind, in.kind, sizeof(kind));
    memcpy(crop, in.crop, sizeof(crop));
    memcpy(waterLeft, in.water, sizeof(waterLeft));
    memcpy(growth, in.growth, sizeof(growth));
    for (uint16_t i = 0; i < TILE_COUNT; i++)
    {
        if (kind[i] >= (uint8_t)TileKind::COUNT)
        {
            kind[i] = (uint8_t)TileKind::GRASS;
        }
        if (crop[i] >= CROP_COUNT)
        {
            crop[i] = 0;
        }
        if (growth[i] > GROWTH_FULL)
        {
            growth[i] = GROWTH_FULL;
        }
    }
    markAllChanged();
}
