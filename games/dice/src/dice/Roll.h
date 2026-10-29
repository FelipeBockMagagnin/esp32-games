#ifndef DICE_ROLL_H
#define DICE_ROLL_H

#include <stddef.h>
#include <stdint.h>
#include "Random.h"

static const uint8_t MAX_DICE = 10;
static const uint8_t DIE_SIDES[] = {2, 4, 6, 8, 10, 12, 20, 100};
static const uint8_t DIE_TYPE_COUNT = sizeof(DIE_SIDES) / sizeof(DIE_SIDES[0]);
static const uint8_t DEFAULT_DIE = 6;
static const int8_t MAX_BONUS = 20;

// What the pool adds up. The keeps set one die aside each way: two d20 kept high is
// advantage, four d6 dropping the lowest is the classic ability score.
enum class Keep : uint8_t
{
    SUM,
    HIGHEST,
    LOWEST,
    DROP_LOWEST,
    COUNT
};

struct RollSpec
{
    uint8_t count; // 1..MAX_DICE
    uint8_t die;   // Index into DIE_SIDES
    int8_t bonus;  // -MAX_BONUS..MAX_BONUS
    uint8_t keep;  // Keep
};

enum class Outcome : uint8_t
{
    PLAIN,
    CRITICAL, // A lone kept d20 showing 20
    FUMBLE,   // A lone kept d20 showing 1
    MAX,      // Every kept die at its top face
    MIN       // Every kept die on 1
};

struct RollResult
{
    RollSpec spec;
    uint8_t faces[MAX_DICE];
    bool kept[MAX_DICE];
    int16_t total;
    Outcome outcome;
};

void defaultSpec(RollSpec &spec);
// Pulls every field back into range, so a spec read from flash is always safe to roll
void clampSpec(RollSpec &spec);
inline uint8_t sidesOf(const RollSpec &spec) { return DIE_SIDES[spec.die < DIE_TYPE_COUNT ? spec.die : DEFAULT_DIE]; }
// A keep only means something with two dice or more
inline Keep keepOf(const RollSpec &spec) { return spec.count > 1 ? (Keep)spec.keep : Keep::SUM; }

const char *keepName(Keep keep);
// Dice notation: 2d6+3, 2d20kh1, 4d6dl1-1
void formatSpec(const RollSpec &spec, char *buf, size_t len);
// The faces in order, a set-aside die in brackets: "14 [9]"
void formatFaces(const RollResult &result, char *buf, size_t len);

void rollDice(const RollSpec &spec, RandomSource source, RollResult &out);
// Which dice count and what they add up to, for faces already on the table
void scoreDice(RollResult &result);

#endif // DICE_ROLL_H
