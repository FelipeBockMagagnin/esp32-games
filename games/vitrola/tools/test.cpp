// Logic tests on the PC: every track decodes to its stated length, the playlist's modes, the
// transport's verbs and the save. Build and run with tools/test.sh.
#include <cassert>
#include <cstdio>
#include <set>
#include "../src/app/Playlist.h"
#include "../src/app/Save.h"
#include "../src/app/Transport.h"
#include "../src/audio/Player.h"
#include <Preferences.h>

long random(long max) { return max > 0 ? rand() % max : 0; }
long random(long min, long max) { return min + random(max - min); }

static uint32_t state = 0x12345678u;
static uint32_t xorshift()
{
    state ^= state << 13;
    state ^= state >> 17;
    state ^= state << 5;
    return state;
}

static int saves = 0;
static void counted() { saves++; }

static void tracksDecode()
{
    uint32_t albumFrames[64] = {};
    for (uint16_t t = 0; t < TRACK_COUNT; t++)
    {
        const Track &track = TRACKS[t];
        assert(track.data[0] == FRAME_RATE);
        assert(track.album < ALBUM_COUNT);
        TrackReader reader;
        reader.start(track.data, track.size);
        uint16_t period;
        uint32_t frames = 0, sounding = 0;
        while (reader.step(period))
        {
            frames++;
            if (period != 0)
            {
                sounding++;
                uint32_t hz = PIT_CLOCK / period;
                assert(hz >= 20 && hz <= 20000);
            }
        }
        assert(reader.hasEnded());
        assert(frames == track.frames);
        assert(sounding > frames / 4);
        albumFrames[track.album] += frames;
    }
    for (uint8_t a = 0; a < ALBUM_COUNT; a++)
    {
        assert(ALBUMS[a].frames == albumFrames[a]);
        assert(ALBUMS[a].count > 0);
        assert(a == 0 || ALBUMS[a].first == ALBUMS[a - 1].first + ALBUMS[a - 1].count);
    }
    assert(ALBUMS[ALBUM_COUNT - 1].first + ALBUMS[ALBUM_COUNT - 1].count == TRACK_COUNT);
    printf("ok  %u tracks decode to their lengths\n", TRACK_COUNT);
}

static void playlistModes()
{
    Playlist list(xorshift);
    const Album &a = ALBUMS[1];
    list.current = a.first + a.count - 1;
    list.mode = Mode::ALBUM;
    assert(list.afterEnd() == -1);
    assert(list.next() == a.first);
    list.current = a.first;
    assert(list.previous() == a.first + a.count - 1);
    assert(list.afterEnd() == a.first + 1);
    list.mode = Mode::REPEAT;
    list.current = a.first + a.count - 1;
    assert(list.afterEnd() == a.first);
    list.mode = Mode::ONE;
    assert(list.afterEnd() == list.current);
    list.mode = Mode::SHUFFLE;
    std::set<uint16_t> seen;
    for (int i = 0; i < 2000; i++)
    {
        int32_t n = list.afterEnd();
        assert(n >= 0 && n < TRACK_COUNT && n != list.current);
        seen.insert((uint16_t)n);
    }
    assert(seen.size() == (size_t)TRACK_COUNT - 1);
    list.current = a.first + 2;
    uint32_t before = 0;
    for (uint16_t t = a.first; t < a.first + 2; t++)
    {
        before += TRACKS[t].frames;
    }
    assert(list.framesBefore() == before);
    printf("ok  playlist modes\n");
}

static void transportVerbs()
{
    Player player;
    Playlist list(xorshift);
    Transport transport(player, list, counted);
    assert(transport.needle() < 0);
    transport.toggle();
    assert(player.isPlaying() && list.current == 0 && saves == 1);
    for (int i = 0; i < 10 * FRAME_RATE; i++)
    {
        player.tick();
    }
    assert(player.getFrame() == 10u * FRAME_RATE);
    float n = transport.needle();
    assert(n > 0 && n < 0.05f);
    transport.toggle();
    assert(player.isPaused());
    player.tick();
    assert(player.getFrame() == 10u * FRAME_RATE && player.getPeriod() == 0);
    transport.previous();
    assert(list.current == 0 && player.getFrame() == 0);
    transport.previous();
    assert(list.current == ALBUMS[0].count - 1);
    transport.next();
    assert(list.current == 0);

    list.mode = Mode::ALBUM;
    transport.play(ALBUMS[0].first + ALBUMS[0].count - 1);
    while (player.isLoaded())
    {
        player.tick();
    }
    transport.update();
    assert(!player.isLoaded() && transport.needle() < 0);
    transport.play(0);
    while (player.isLoaded())
    {
        player.tick();
    }
    transport.update();
    assert(player.isPlaying() && list.current == 1);
    int before = saves;
    transport.cycleMode();
    assert(list.mode == Mode::REPEAT && saves == before + 1);
    printf("ok  transport verbs and the end of a side\n");
}

static void saveRoundTrip()
{
    SlotStore store("vitrola");
    SaveData save, back;
    defaultSave(save);
    save.track = 7;
    save.mode = (uint8_t)Mode::SHUFFLE;
    assert(writeSave(store, save));
    defaultSave(back);
    assert(readSave(store, back) && back.track == 7 && back.mode == (uint8_t)Mode::SHUFFLE);
    save.track = 60000;
    save.mode = 99;
    assert(writeSave(store, save));
    assert(readSave(store, back) && back.track == 0 && back.mode == 0);
    printf("ok  save round trip\n");
}

int main()
{
    tracksDecode();
    playlistModes();
    transportVerbs();
    saveRoundTrip();
    printf("all passed\n");
}
