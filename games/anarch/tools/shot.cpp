// Runs the game on the PC with the firmware's settings: taps A through the menu into level 1,
// plays the moves given, and writes the last frame doubled to 320x240 as on the board.
//   shot <out.png> [key:frames]...   keys: up down left right a c (c + up jumps), wait
// Fails if the level never starts or the game exits.
#include <cstdio>
#include <cstdlib>
#include <cstring>
#include <string>
#include "../src/AnarchConfig.h"
#include "Png.h"

bool launcherShown = false;

static const int W = SFG_SCREEN_RESOLUTION_X;
static const int H = SFG_SCREEN_RESOLUTION_Y;
static uint8_t screen[W * H];
static int8_t keys[SFG_KEY_COUNT];
static uint32_t now = 0;

void SFG_setPixel(uint16_t x, uint16_t y, uint8_t colorIndex) { screen[y * W + x] = colorIndex; }
uint32_t SFG_getTimeMs() { return now; }
void SFG_sleepMs(uint16_t) {}
void SFG_getMouseOffset(int16_t *, int16_t *) {}
int8_t SFG_keyPressed(uint8_t key) { return key < SFG_KEY_COUNT ? keys[key] : 0; }
void SFG_playSound(uint8_t, uint8_t) {}
void SFG_setMusic(uint8_t) {}
void SFG_processEvent(uint8_t, uint8_t) {}
void SFG_save(uint8_t[SFG_SAVE_SIZE]) {}
uint8_t SFG_load(uint8_t[SFG_SAVE_SIZE]) { return 1; }

static bool step()
{
    now += SFG_MS_PER_FRAME;
    return SFG_mainLoopBody();
}

static int keyIndex(const std::string &name)
{
    const char *names[] = {"up", "right", "down", "left", "a", "b", "c"};
    for (int i = 0; i < 7; i++)
    {
        if (name == names[i])
        {
            return i;
        }
    }
    return -1;
}

int main(int argc, char **argv)
{
    if (argc < 2)
    {
        fprintf(stderr, "usage: shot <out.png> [key:frames]...\n");
        return 2;
    }
    SFG_init();
    int frames = 0;
    while (SFG_game.state != SFG_GAME_STATE_PLAYING)
    {
        if (++frames > 3000)
        {
            fprintf(stderr, "level 1 never started (state %d)\n", SFG_game.state);
            return 1;
        }
        keys[SFG_KEY_A] = frames / 10 % 2;
        if (!step())
        {
            fprintf(stderr, "the game exited\n");
            return 1;
        }
    }
    memset(keys, 0, sizeof(keys));
    for (int i = 2; i < argc; i++)
    {
        std::string arg = argv[i];
        size_t colon = arg.find(':');
        std::string name = arg.substr(0, colon);
        int count = colon == std::string::npos ? 1 : atoi(arg.c_str() + colon + 1);
        memset(keys, 0, sizeof(keys));
        if (name == "c")
        {
            keys[SFG_KEY_C] = keys[SFG_KEY_UP] = 1;
        }
        else if (name != "wait")
        {
            int k = keyIndex(name);
            if (k < 0)
            {
                fprintf(stderr, "unknown key %s\n", name.c_str());
                return 2;
            }
            keys[k] = 1;
        }
        for (int f = 0; f < count; f++)
        {
            if (!step())
            {
                fprintf(stderr, "the game exited\n");
                return 1;
            }
        }
    }

    static uint16_t out[W * 2 * H * 2];
    for (int y = 0; y < H * 2; y++)
    {
        for (int x = 0; x < W * 2; x++)
        {
            out[y * W * 2 + x] = paletteRGB565[screen[y / 2 * W + x / 2]];
        }
    }
    writePng(argv[1], out, W * 2, H * 2);
    printf("level %d, state %d, health %d, frame %u\n", SFG_currentLevel.levelNumber, SFG_game.state,
           SFG_player.health, (unsigned)SFG_game.frame);
    return 0;
}
