#ifndef COMBAT_STYLE_H
#define COMBAT_STYLE_H

#include "../../ui/Theme.h"

namespace CombatStyle
{
static const uint16_t READY = Theme::SELECT;  // Ring and outline of a pressable action
static const uint16_t ACTIVE = Theme::INFO;   // Guard and ward while they are up
static const uint16_t BLOCKED = Theme::BAD;
}

#endif // COMBAT_STYLE_H
