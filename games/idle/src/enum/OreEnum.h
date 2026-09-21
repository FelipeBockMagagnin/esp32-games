#ifndef ORE_ENUM_H
#define ORE_ENUM_H

#include <Arduino.h>

enum class OreEnum : uint8_t
{
    COPPER,
    SILVER,
    GOLD,
    COUNT
};

struct OreInfo
{
    const char *name;
    uint16_t color;
};

inline const OreInfo &getOreInfo(OreEnum ore)
{
    static const OreInfo table[] = {
        {"Copper", 0xE300},
        {"Silver", 0x73AF},
        {"Gold", 0xF685},
    };
    uint8_t index = static_cast<uint8_t>(ore);
    return table[index < static_cast<uint8_t>(OreEnum::COUNT) ? index : 0];
}

#endif // ORE_ENUM_H
