#ifndef TRANSPORT_H
#define TRANSPORT_H

#include "Playlist.h"
#include "../audio/Player.h"

// The buttons' verbs over the player and the playlist, and what happens when a track runs out.
// Every change of track or mode calls `changed`, to save them.
class Transport
{
public:
    Transport(Player &player, Playlist &playlist, void (*changed)())
        : player(player), playlist(playlist), changed(changed) {}

    void play(uint16_t track);
    void toggle();
    void next();
    void previous();
    void cycleMode();
    // Call every loop: starts the next track when one ends
    void update();

    // 0..1 across the record's side, for the needle; negative when the arm should rest
    float needle() const;

    Player &player;
    Playlist &playlist;

private:
    void (*changed)();
};

#endif // TRANSPORT_H
