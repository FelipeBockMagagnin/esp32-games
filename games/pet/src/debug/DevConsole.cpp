#ifdef DEV_MODE

#include "DevConsole.h"

DevConsole::DevConsole(World &world, Colony &colony, void (*changed)())
    : world(world), colony(colony), changed(changed), length(0)
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

void DevConsole::run(char *command)
{
    char *name = strtok(command, " ");
    char *arg = strtok(nullptr, " ");
    unsigned long value = arg != nullptr ? strtoul(arg, nullptr, 10) : 0;
    captureSave(blob, world, colony);
    World::Snapshot &w = blob.world;

    if (strcmp(name, "coins") == 0)
    {
        w.coins = value;
    }
    else if (strcmp(name, "food") == 0)
    {
        w.food = value;
    }
    else if (strcmp(name, "ripe") == 0)
    {
        for (uint16_t i = 0; i < TILE_COUNT; i++)
        {
            if (w.kind[i] == (uint8_t)TileKind::CROP)
            {
                w.growth[i] = GROWTH_FULL;
            }
        }
    }
    else if (strcmp(name, "open") == 0)
    {
        for (uint16_t i = 0; i < TILE_COUNT; i++)
        {
            if (w.kind[i] == (uint8_t)TileKind::WILD)
            {
                w.kind[i] = (uint8_t)TileKind::GRASS;
            }
        }
    }
    else if (strcmp(name, "unlock") == 0)
    {
        w.crops = 0xFF;
        w.upgrades = 0xFFFF;
    }
    else if (strcmp(name, "hungry") == 0)
    {
        for (uint8_t i = 0; i < blob.colony.count; i++)
        {
            blob.colony.critters[i].hunger = NEED_MAX;
        }
    }
    else
    {
        Serial.println("coins N | food N | ripe | open | unlock | hungry");
        return;
    }
    applySave(blob, world, colony);
    changed();
    Serial.println("ok");
}

#endif // DEV_MODE
