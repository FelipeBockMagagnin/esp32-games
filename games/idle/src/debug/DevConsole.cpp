#ifdef DEV_MODE

#include "DevConsole.h"
#include "../game/Format.h"

// Accepts 1e9 and the suffixes k, m, b, t
static uint64_t parseAmount(const char *text)
{
    if (text == nullptr)
    {
        return 0;
    }
    char *end;
    double value = strtod(text, &end);
    switch (tolower(*end))
    {
    case 'k': value *= 1e3; break;
    case 'm': value *= 1e6; break;
    case 'b': value *= 1e9; break;
    case 't': value *= 1e12; break;
    }
    if (value <= 0)
    {
        return 0;
    }
    return value >= 1.8e19 ? UINT64_MAX : (uint64_t)value;
}

DevConsole::DevConsole(GameState &game, Inventory &inventory, CombatState &combat, Achievements &achievements,
                       Unlocks &unlocks, Ascension &ascension, Journal &journal, void (*changed)())
    : game(game), inventory(inventory), combat(combat), achievements(achievements), unlocks(unlocks),
      ascension(ascension), journal(journal), changed(changed), length(0)
{
}

void DevConsole::update()
{
    while (Serial.available() > 0)
    {
        char c = Serial.read();
        if (c == '\n' || c == '\r')
        {
            if (length > 0)
            {
                line[length] = '\0';
                length = 0;
                run(line);
            }
        }
        else if (length < sizeof(line) - 1)
        {
            line[length++] = c;
        }
    }
}

void DevConsole::capture()
{
    captureSave(blob, game, inventory, combat, achievements, unlocks, ascension, journal);
}

void DevConsole::apply()
{
    applySave(blob, game, inventory, combat, achievements, unlocks, ascension, journal);
    changed();
}

void DevConsole::run(char *command)
{
    const char *name = strtok(command, " ");
    if (name == nullptr)
    {
        return;
    }
    const char *arg = strtok(nullptr, " ");
    uint64_t n = parseAmount(arg);

    if (strcmp(name, "gold") == 0)
    {
        game.addGold(n);
        Serial.printf("gold: %s\n", formatAmount(game.getGold()).c_str());
    }
    else if (strcmp(name, "level") == 0)
    {
        capture();
        blob.game.miningLevel = n > 0 && n <= UINT16_MAX ? (uint16_t)n : 1;
        blob.game.miningXp = 0;
        apply();
        Serial.printf("level: %u\n", game.getMiningLevel());
    }
    else if (strcmp(name, "zone") == 0)
    {
        uint8_t last = n < ZONE_COUNT ? (uint8_t)n : ZONE_COUNT - 1;
        capture();
        for (uint8_t i = 1; i <= last; i++)
        {
            const ZoneDef &zone = ZONES[i];
            if (blob.combat.zoneKills[i - 1] < zone.reqPrevZoneKills)
            {
                blob.combat.zoneKills[i - 1] = zone.reqPrevZoneKills;
            }
            for (uint8_t u = 0; zone.requiresUnlockUpgrade && u < GOLD_UPGRADE_COUNT; u++)
            {
                if (GOLD_UPGRADES[u].target == UpgradeTarget::UNLOCK_ZONE && GOLD_UPGRADES[u].bonusPercent == i)
                {
                    blob.game.goldUpgradesBought[u] = true;
                }
            }
            uint8_t &delve = blob.ascension.talentLevels[TALENT_DEEP_DELVER];
            if (delve < zone.reqDelveLevel)
            {
                delve = zone.reqDelveLevel;
            }
        }
        for (uint8_t i = 0; i < FEATURE_COUNT; i++)
        {
            blob.unlocks.unlocked[i] = true;
        }
        apply();
        Serial.printf("zones 0-%u open, up to %s\n", last, ZONES[last].name);
    }
    else if (strcmp(name, "unlock") == 0)
    {
        capture();
        for (uint8_t i = 0; i < FEATURE_COUNT; i++)
        {
            blob.unlocks.unlocked[i] = true;
        }
        apply();
        Serial.println("all screens open");
    }
    else if (strcmp(name, "items") == 0)
    {
        uint8_t level = n < ITEM_MAX_LEVEL ? (uint8_t)n : ITEM_MAX_LEVEL;
        capture();
        for (uint8_t i = 0; i < ITEM_COUNT; i++)
        {
            if (blob.inventory.itemLevels[i] < level)
            {
                blob.inventory.itemLevels[i] = level;
            }
        }
        apply();
        Serial.printf("items: all at Lv %u or more\n", level);
    }
    else if (strcmp(name, "points") == 0)
    {
        capture();
        uint64_t target = (uint64_t)Ascension::pointsFor(blob.ascension.priorUnits) + n;
        if (target > 2642245) // Cube root of UINT64_MAX, as in Ascension::pointsFor
        {
            target = 2642245;
        }
        uint64_t units = target * target * target;
        if (blob.ascension.priorUnits < units)
        {
            blob.ascension.priorUnits = units;
        }
        apply();
        Serial.printf("points: %u available\n", ascension.getPointsAvailable());
    }
    else
    {
        Serial.println("gold <n> | level <n> | zone <n> | unlock | items <lv> | points <n>   (n: 5k 2m 3b 1t 1e15)");
    }
}

#endif // DEV_MODE
