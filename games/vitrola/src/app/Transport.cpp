#include "Transport.h"

void Transport::play(uint16_t track)
{
    if (track >= TRACK_COUNT)
    {
        return;
    }
    playlist.current = track;
    player.play(track);
    changed();
}

void Transport::toggle()
{
    if (!player.isLoaded())
    {
        play(playlist.current);
        return;
    }
    player.setPaused(!player.isPaused());
}

void Transport::next()
{
    play(playlist.next());
}

void Transport::previous()
{
    // Like a CD player: past the first seconds the track starts over
    if (player.isLoaded() && player.getFrame() > 3u * FRAME_RATE)
    {
        play(playlist.current);
        return;
    }
    play(playlist.previous());
}

void Transport::cycleMode()
{
    playlist.mode = (Mode)(((uint8_t)playlist.mode + 1) % (uint8_t)Mode::COUNT);
    changed();
}

void Transport::update()
{
    if (!player.takeEnded())
    {
        return;
    }
    int32_t after = playlist.afterEnd();
    if (after >= 0)
    {
        play((uint16_t)after);
    }
}

float Transport::needle() const
{
    if (!player.isLoaded())
    {
        return -1;
    }
    const Album &a = ALBUMS[playlist.album()];
    if (a.frames == 0)
    {
        return 0;
    }
    return (float)(playlist.framesBefore() + player.getFrame()) / a.frames;
}
