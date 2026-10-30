#ifndef ALLOCATION_H
#define ALLOCATION_H

#include <math.h>
#include <stdio.h>
#include "../nan/Format.h"
#include "../nan/Game.h"

// What Confirm does on a training or augment row, picked on the screens' first row and shared
// between them
enum class AllocMode : uint8_t
{
    ADD_STEP,
    ADD_HALF,
    ADD_ALL,
    TAKE_STEP,
    TAKE_ALL,
    COUNT
};

struct Allocation
{
    AllocMode mode = AllocMode::ADD_STEP;

    void next() { mode = (AllocMode)(((uint8_t)mode + 1) % (uint8_t)AllocMode::COUNT); }

    void describe(const Game &game, char *out, size_t size) const
    {
        char step[12];
        formatNumber(game.allocStep(), step, sizeof(step));
        switch (mode)
        {
        case AllocMode::ADD_STEP:
            snprintf(out, size, "Acao: +%s energia", step);
            break;
        case AllocMode::ADD_HALF:
            snprintf(out, size, "Acao: +metade da livre");
            break;
        case AllocMode::ADD_ALL:
            snprintf(out, size, "Acao: +toda a livre");
            break;
        case AllocMode::TAKE_STEP:
            snprintf(out, size, "Acao: -%s energia", step);
            break;
        default:
            snprintf(out, size, "Acao: -tudo da linha");
            break;
        }
    }

    // The energy moved, 0 when nothing could be
    double apply(Game &game, Game::Target target) const
    {
        switch (mode)
        {
        case AllocMode::ADD_STEP:
            return game.allocate(target, game.allocStep());
        case AllocMode::ADD_HALF:
            return game.allocate(target, game.idleEnergy() >= 2 ? floor(game.idleEnergy() / 2) : game.idleEnergy());
        case AllocMode::ADD_ALL:
            return game.allocate(target, game.idleEnergy());
        case AllocMode::TAKE_STEP:
            return game.reclaim(target, game.allocStep());
        default:
            return game.reclaim(target, game.allocatedTo(target));
        }
    }
};

#endif // ALLOCATION_H
