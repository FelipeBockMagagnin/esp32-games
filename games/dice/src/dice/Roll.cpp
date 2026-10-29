#include "Roll.h"
#include <stdio.h>
#include <string.h>

static const char *const KEEP_NAMES[(uint8_t)Keep::COUNT] = {"Sum", "Keep high", "Keep low", "Drop low"};
static const char *const KEEP_NOTATION[(uint8_t)Keep::COUNT] = {"", "kh1", "kl1", "dl1"};

void defaultSpec(RollSpec &spec)
{
    spec = {1, DEFAULT_DIE, 0, (uint8_t)Keep::SUM};
}

void clampSpec(RollSpec &spec)
{
    if (spec.count < 1 || spec.count > MAX_DICE)
    {
        spec.count = 1;
    }
    if (spec.die >= DIE_TYPE_COUNT)
    {
        spec.die = DEFAULT_DIE;
    }
    if (spec.bonus < -MAX_BONUS || spec.bonus > MAX_BONUS)
    {
        spec.bonus = 0;
    }
    if (spec.keep >= (uint8_t)Keep::COUNT)
    {
        spec.keep = (uint8_t)Keep::SUM;
    }
}

const char *keepName(Keep keep)
{
    return KEEP_NAMES[(uint8_t)keep < (uint8_t)Keep::COUNT ? (uint8_t)keep : 0];
}

void formatSpec(const RollSpec &spec, char *buf, size_t len)
{
    int n = snprintf(buf, len, "%ud%u%s", (unsigned)spec.count, (unsigned)sidesOf(spec),
                     KEEP_NOTATION[(uint8_t)keepOf(spec)]);
    if (spec.bonus != 0 && n > 0 && (size_t)n < len)
    {
        snprintf(buf + n, len - n, "%+d", (int)spec.bonus);
    }
}

void formatFaces(const RollResult &result, char *buf, size_t len)
{
    size_t used = 0;
    buf[0] = '\0';
    for (uint8_t i = 0; i < result.spec.count && used + 1 < len; i++)
    {
        const char *format = result.kept[i] ? "%s%u" : "%s[%u]";
        int n = snprintf(buf + used, len - used, format, i == 0 ? "" : " ", (unsigned)result.faces[i]);
        if (n < 0)
        {
            break;
        }
        used += (size_t)n;
    }
}

void scoreDice(RollResult &result)
{
    const RollSpec &spec = result.spec;
    uint8_t sides = sidesOf(spec);
    Keep keep = keepOf(spec);

    int8_t aside = -1;
    if (keep != Keep::SUM)
    {
        uint8_t best = 0;
        for (uint8_t i = 1; i < spec.count; i++)
        {
            bool better = keep == Keep::HIGHEST ? result.faces[i] > result.faces[best]
                                                : result.faces[i] < result.faces[best];
            if (better)
            {
                best = i;
            }
        }
        aside = (int8_t)best;
    }

    int16_t sum = 0;
    uint8_t keptCount = 0, atTop = 0, atOne = 0;
    uint8_t lone = 0;
    for (uint8_t i = 0; i < spec.count; i++)
    {
        bool kept;
        if (keep == Keep::SUM)
        {
            kept = true;
        }
        else if (keep == Keep::DROP_LOWEST)
        {
            kept = i != aside;
        }
        else
        {
            kept = i == aside;
        }
        result.kept[i] = kept;
        if (!kept)
        {
            continue;
        }
        sum += result.faces[i];
        keptCount++;
        lone = result.faces[i];
        atTop += result.faces[i] == sides;
        atOne += result.faces[i] == 1;
    }
    for (uint8_t i = spec.count; i < MAX_DICE; i++)
    {
        result.faces[i] = 0;
        result.kept[i] = false;
    }
    result.total = sum + spec.bonus;

    if (sides == 20 && keptCount == 1)
    {
        result.outcome = lone == 20 ? Outcome::CRITICAL : (lone == 1 ? Outcome::FUMBLE : Outcome::PLAIN);
    }
    else if (atTop == keptCount)
    {
        result.outcome = Outcome::MAX;
    }
    else if (atOne == keptCount)
    {
        result.outcome = Outcome::MIN;
    }
    else
    {
        result.outcome = Outcome::PLAIN;
    }
}

void rollDice(const RollSpec &spec, RandomSource source, RollResult &out)
{
    memset(&out, 0, sizeof(out));
    out.spec = spec;
    clampSpec(out.spec);
    uint8_t sides = sidesOf(out.spec);
    for (uint8_t i = 0; i < out.spec.count; i++)
    {
        out.faces[i] = (uint8_t)(1 + uniform(source, sides));
    }
    scoreDice(out);
}
