#include "Playlist.h"

static const char *const NAMES[] = {"Disco", "Repetir", "Faixa", "Aleatorio"};
static const char *const TAGS[] = {"DSC", "REP", "UMA", "ALE"};

const char *modeName(Mode m)
{
    return NAMES[(uint8_t)m < (uint8_t)Mode::COUNT ? (uint8_t)m : 0];
}

const char *modeTag(Mode m)
{
    return TAGS[(uint8_t)m < (uint8_t)Mode::COUNT ? (uint8_t)m : 0];
}

uint32_t Playlist::framesBefore() const
{
    uint32_t total = 0;
    for (uint16_t t = ALBUMS[album()].first; t < current; t++)
    {
        total += TRACKS[t].frames;
    }
    return total;
}

uint16_t Playlist::draw() const
{
    if (TRACK_COUNT < 2)
    {
        return current;
    }
    uint16_t pick = (uint16_t)(random() % (TRACK_COUNT - 1));
    return pick >= current ? pick + 1 : pick;
}

uint16_t Playlist::next() const
{
    if (mode == Mode::SHUFFLE)
    {
        return draw();
    }
    const Album &a = ALBUMS[album()];
    return a.first + (index() + 1) % a.count;
}

uint16_t Playlist::previous() const
{
    if (mode == Mode::SHUFFLE)
    {
        return draw();
    }
    const Album &a = ALBUMS[album()];
    return a.first + (index() + a.count - 1) % a.count;
}

int32_t Playlist::afterEnd() const
{
    switch (mode)
    {
    case Mode::ONE:
        return current;
    case Mode::SHUFFLE:
        return draw();
    case Mode::REPEAT:
        return next();
    default:
        return index() + 1 < ALBUMS[album()].count ? current + 1 : -1;
    }
}
