#ifndef GAME_FORMAT_H
#define GAME_FORMAT_H

#include <Arduino.h>
#include "../enum/OreEnum.h"

// "999", "12.3K", "4.5M"... fits the narrow columns of the 240px screen
String formatAmount(uint64_t value);

// "+0.1 copper/s" from a rate in ORE_SCALE units per second
String formatRate(uint32_t perSecond, OreEnum ore);

// "+0.1/s", short form for narrow spots
String formatPerSecond(uint32_t perSecond);

#endif // GAME_FORMAT_H
