#include "Ascension.h"
#include <math.h>

Ascension::Ascension()
    : priorUnits(0),
      ascensions(0),
      pointsSpent(0),
      revision(0)
{
    for (uint8_t i = 0; i < TALENT_COUNT; i++)
    {
        talentLevels[i] = 0;
    }
    recount();
}

// The largest cube that fits in 64 bits is this number's
static const uint64_t MAX_CUBE_ROOT = 2642245;

// The float estimate is nudged to the exact integer root, which a double can miss by one
// near a perfect cube
uint32_t Ascension::pointsFor(uint64_t units)
{
    uint64_t root = (uint64_t)cbrt((double)units);
    if (root > MAX_CUBE_ROOT)
    {
        root = MAX_CUBE_ROOT;
    }
    while (root > 0 && root * root * root > units)
    {
        root--;
    }
    while (root < MAX_CUBE_ROOT && (root + 1) * (root + 1) * (root + 1) <= units)
    {
        root++;
    }
    return (uint32_t)root;
}

uint64_t Ascension::getLifetimeUnits(uint64_t runGold) const
{
    uint64_t runUnits = runGold / ASCEND_GOLD_PER_POINT;
    return priorUnits > UINT64_MAX - runUnits ? UINT64_MAX : priorUnits + runUnits;
}

uint64_t Ascension::getLifetimeGold(uint64_t runGold) const
{
    uint64_t prior = priorUnits > UINT64_MAX / ASCEND_GOLD_PER_POINT ? UINT64_MAX : priorUnits * ASCEND_GOLD_PER_POINT;
    return prior > UINT64_MAX - runGold ? UINT64_MAX : prior + runGold;
}

uint32_t Ascension::getPointsAvailable() const
{
    uint32_t earned = getPointsEarned();
    // A retuned price can leave a save having spent more than it has
    return earned > pointsSpent ? earned - pointsSpent : 0;
}

uint32_t Ascension::getPendingPoints(uint64_t runGold) const
{
    return pointsFor(getLifetimeUnits(runGold)) - getPointsEarned();
}

uint64_t Ascension::getRunGoldForNextPoint(uint64_t runGold) const
{
    uint64_t next = pointsFor(getLifetimeUnits(runGold)) + 1;
    if (next > MAX_CUBE_ROOT)
    {
        return UINT64_MAX;
    }
    uint64_t needed = next * next * next - priorUnits;
    return needed > UINT64_MAX / ASCEND_GOLD_PER_POINT ? UINT64_MAX : needed * ASCEND_GOLD_PER_POINT;
}

bool Ascension::ascend(uint64_t runGold)
{
    if (getPendingPoints(runGold) == 0)
    {
        return false;
    }
    priorUnits = getLifetimeUnits(runGold);
    ascensions++;
    revision++;
    return true;
}

// Each level costs costGrowthPercent of the one before, rounded up so a cheap talent's
// price still climbs; saturates instead of wrapping
uint32_t Ascension::talentCost(const TalentDef &def, uint8_t level)
{
    uint64_t cost = def.baseCost;
    for (uint8_t i = 0; i < level && cost < UINT32_MAX; i++)
    {
        cost = (cost * def.costGrowthPercent + 99) / 100;
    }
    return cost < UINT32_MAX ? (uint32_t)cost : UINT32_MAX;
}

uint8_t Ascension::getTalentLevel(uint8_t id) const
{
    return id < TALENT_COUNT ? talentLevels[id] : 0;
}

bool Ascension::isTalentOpen(uint8_t id) const
{
    if (id >= TALENT_COUNT)
    {
        return false;
    }
    uint8_t parent = TALENTS[id].parent;
    return parent == NO_TALENT || getTalentLevel(parent) > 0;
}

bool Ascension::isTalentMaxed(uint8_t id) const
{
    return id < TALENT_COUNT && talentLevels[id] >= TALENTS[id].maxLevel;
}

uint32_t Ascension::getTalentCost(uint8_t id) const
{
    if (id >= TALENT_COUNT)
    {
        return 0;
    }
    return talentCost(TALENTS[id], talentLevels[id]);
}

bool Ascension::canBuyTalent(uint8_t id) const
{
    return isTalentOpen(id) && !isTalentMaxed(id) && getPointsAvailable() >= getTalentCost(id);
}

bool Ascension::buyTalent(uint8_t id)
{
    if (!canBuyTalent(id))
    {
        return false;
    }
    talentLevels[id]++;
    recount();
    revision++;
    return true;
}

uint8_t Ascension::getTalentDepth(uint8_t id) const
{
    uint8_t depth = 0;
    while (id < TALENT_COUNT && TALENTS[id].parent != NO_TALENT && depth < TALENT_COUNT)
    {
        id = TALENTS[id].parent;
        depth++;
    }
    return depth;
}

void Ascension::recount()
{
    pointsSpent = 0;
    for (uint8_t i = 0; i < TALENT_EFFECT_COUNT; i++)
    {
        effectPercent[i] = 0;
        effectLevel[i] = 0;
    }
    for (uint8_t i = 0; i < TALENT_COUNT; i++)
    {
        const TalentDef &def = TALENTS[i];
        for (uint8_t level = 0; level < talentLevels[i]; level++)
        {
            pointsSpent += talentCost(def, level);
        }
        effectPercent[(uint8_t)def.effect] += (uint32_t)def.perLevel * talentLevels[i];
        effectLevel[(uint8_t)def.effect] += talentLevels[i];
    }
}

void Ascension::save(Snapshot &out) const
{
    out.priorUnits = priorUnits;
    out.ascensions = ascensions;
    for (uint8_t i = 0; i < TALENT_COUNT; i++)
    {
        out.talentLevels[i] = talentLevels[i];
    }
}

void Ascension::load(const Snapshot &in)
{
    priorUnits = in.priorUnits;
    ascensions = in.ascensions;
    for (uint8_t i = 0; i < TALENT_COUNT; i++)
    {
        talentLevels[i] = in.talentLevels[i] < TALENTS[i].maxLevel ? in.talentLevels[i] : TALENTS[i].maxLevel;
    }
    recount();
    revision++;
}
