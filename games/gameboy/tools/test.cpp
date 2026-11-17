// Logic tests on the PC: the pack, the save codec and store, the scaler, the sound model, and
// every ROM in library.json booting in Peanut-GB through GameBoy. Run by tools/test.sh.
#include <Preferences.h>
#include <cstdio>
#include <fstream>
#include <iterator>
#include <string>
#include <vector>
#include "../src/emu/Apu.h"
#include "../src/emu/GameBoy.h"
#include "../src/emu/Pack.h"
#include "../src/emu/SaveCodec.h"
#include "../src/emu/Saves.h"
#include "../src/emu/Scale.h"

long random(long max) { return max > 0 ? rand() % max : 0; }
long random(long min, long max) { return min + random(max - min); }

static int failures = 0;

#define CHECK(cond)                                                       \
    do                                                                    \
    {                                                                     \
        if (!(cond))                                                      \
        {                                                                 \
            printf("FAIL %s:%d: %s\n", __FILE__, __LINE__, #cond);        \
            failures++;                                                   \
        }                                                                 \
    } while (0)

static void put32(std::vector<uint8_t> &b, uint32_t v)
{
    for (int i = 0; i < 4; i++)
    {
        b.push_back((uint8_t)(v >> (8 * i)));
    }
}

// The layout tools/pack.py writes
static std::vector<uint8_t> makePack(const std::vector<std::vector<uint8_t>> &roms)
{
    std::vector<uint8_t> b = {'G', 'B', 'P', 'K', (uint8_t)roms.size(), 0, 1, 0};
    size_t at = 8 + 64 * roms.size();
    std::vector<size_t> offsets;
    for (auto &r : roms)
    {
        at += -at % 4;
        offsets.push_back(at);
        at += r.size();
    }
    for (size_t i = 0; i < roms.size(); i++)
    {
        put32(b, (uint32_t)offsets[i]);
        put32(b, (uint32_t)roms[i].size());
        put32(b, 0x1000 + (uint32_t)i);
        b.insert(b.end(), {(uint8_t)(i == 1), 0, 0, 0});
        char text[48] = {};
        snprintf(text, 24, "Game %u", (unsigned)i);
        snprintf(text + 24, 24, "Author %u", (unsigned)i);
        b.insert(b.end(), text, text + 48);
    }
    for (size_t i = 0; i < roms.size(); i++)
    {
        b.resize(offsets[i], 0);
        b.insert(b.end(), roms[i].begin(), roms[i].end());
    }
    return b;
}

static void testPack()
{
    std::vector<uint8_t> a(0x200, 0xAA), c(0x301, 0xCC);
    auto blob = makePack({a, c});
    Pack pack;
    CHECK(pack.open(blob.data(), blob.size()));
    CHECK(pack.count() == 2);
    PackEntry e0 = pack.entry(0), e1 = pack.entry(1);
    CHECK(std::string(e0.title) == "Game 0" && std::string(e1.author) == "Author 1");
    CHECK(!e0.okIsB && e1.okIsB);
    CHECK(e0.id == 0x1000 && e1.size == 0x301);
    CHECK(pack.rom(e1)[0] == 0xCC && pack.rom(e1)[0x300] == 0xCC && pack.rom(e0)[0x1FF] == 0xAA);
    CHECK(e1.offset % 4 == 0);

    Pack bad;
    CHECK(!bad.open(blob.data(), blob.size() - 1)); // The last ROM runs past the end
    blob[6] = 2;
    CHECK(!bad.open(blob.data(), blob.size())); // Another version
    CHECK(!bad.open(blob.data(), 4));
}

static void testCodec()
{
    std::vector<uint8_t> ram(0x2000, 0xFF);
    for (int i = 0; i < 300; i++)
    {
        ram[100 + i] = (uint8_t)(i * 7);
    }
    ram[5000] = 1;
    ram[5001] = 1;
    ram[0x1FFF] = 0;
    uint8_t packed[2048];
    size_t n = packSave(ram.data(), ram.size(), packed, sizeof(packed));
    CHECK(n > 0 && n < 500);
    std::vector<uint8_t> back(ram.size(), 0x55);
    CHECK(unpackSave(packed, n, nullptr, back.size()));
    CHECK(unpackSave(packed, n, back.data(), back.size()));
    CHECK(back == ram);
    CHECK(!unpackSave(packed, n, back.data(), back.size() - 1)); // Too much data for the RAM
    CHECK(!unpackSave(packed, n - 1, back.data(), back.size())); // Cut short

    std::vector<uint8_t> noise(0x2000);
    for (auto &b : noise)
    {
        b = (uint8_t)rand();
    }
    CHECK(packSave(noise.data(), noise.size(), packed, sizeof(packed)) == 0);
    n = packSave(noise.data(), 1000, packed, sizeof(packed));
    std::vector<uint8_t> small(1000);
    CHECK(n > 0 && unpackSave(packed, n, small.data(), small.size()));
    CHECK(std::equal(small.begin(), small.end(), noise.begin()));
}

static void testSaves()
{
    Preferences::store.clear();
    std::vector<uint8_t> ram(0x2000, 0);
    ram[10] = 42;
    CHECK(!Saves::exists("gameboy", 7));
    CHECK(Saves::store("gameboy", 7, ram.data(), ram.size()) == Saves::Result::SAVED);
    CHECK(Saves::exists("gameboy", 7));
    std::vector<uint8_t> back(0x2000, 9);
    CHECK(Saves::load("gameboy", 7, back.data(), back.size()));
    CHECK(back == ram);
    std::vector<uint8_t> other(0x800, 9);
    CHECK(!Saves::load("gameboy", 7, other.data(), other.size())); // Another RAM size
    CHECK(other[0] == 9);
    CHECK(!Saves::load("gameboy", 8, back.data(), back.size()));

    std::vector<uint8_t> noise(0x2000);
    for (auto &b : noise)
    {
        b = (uint8_t)rand();
    }
    CHECK(Saves::store("gameboy", 9, noise.data(), noise.size()) == Saves::Result::TOO_BIG);
    Preferences::failWrites = true;
    ram[11] = 1;
    CHECK(Saves::store("gameboy", 7, ram.data(), ram.size()) == Saves::Result::NO_ROOM);
    Preferences::failWrites = false;
}

static void testScale()
{
    uint8_t px[160];
    for (int i = 0; i < 160; i++)
    {
        px[i] = (uint8_t)(i & 3) | 0x20;
    }
    const uint16_t shades[4] = {10, 11, 12, 13};
    uint16_t out[240];
    Scale::line(px, shades, out);
    CHECK(out[0] == 10 && out[1] == 10 && out[2] == 11 && out[3] == 12 && out[5] == 13 && out[239] == 13);
    int rows = 0;
    for (int line = 0; line < 144; line++)
    {
        CHECK(Scale::firstRow(line) == rows);
        rows += Scale::rowCount(line);
    }
    CHECK(rows == Scale::OUT_H);
}

static void testApu()
{
    Apu apu;
    apu.reset();
    CHECK(apu.frame() == 0);
    apu.write(0xFF26, 0x80);
    apu.write(0xFF25, 0xFF);
    // Channel 2 at period 1750: 131072 / 298 = 439 Hz, full volume, no envelope
    apu.write(0xFF17, 0xF0);
    apu.write(0xFF18, 1750 & 0xFF);
    apu.write(0xFF19, 0x80 | 1750 >> 8);
    CHECK(apu.frame() == 439);
    CHECK((apu.read(0xFF26) & 0x02) != 0);
    // A louder channel 1 wins
    apu.write(0xFF12, 0xF0);
    apu.write(0xFF17, 0x80);
    apu.write(0xFF19, 0x80 | 1750 >> 8);
    apu.write(0xFF13, 0x00);
    apu.write(0xFF14, 0x80 | 0x07);
    CHECK(apu.frame() == 131072 / (2048 - 0x700));
    // Panned off, it gives way
    apu.write(0xFF25, 0xEE);
    CHECK(apu.frame() == 439);
    // A length counter silences the channel
    apu.write(0xFF25, 0xFF);
    apu.write(0xFF12, 0x00);
    apu.write(0xFF14, 0x00);
    apu.write(0xFF16, 63);
    apu.write(0xFF19, 0xC0 | 1750 >> 8);
    uint16_t last = 1;
    for (int i = 0; i < 5; i++)
    {
        last = apu.frame();
    }
    CHECK(last == 0);
    apu.write(0xFF26, 0x00);
    CHECK(apu.frame() == 0 && (apu.read(0xFF26) & 0x0F) == 0);
}

struct Frame
{
    int lines = 0;
    bool shades[4] = {};
};

static void sink(void *context, const uint8_t *pixels, uint8_t)
{
    Frame &f = *(Frame *)context;
    f.lines++;
    for (int x = 0; x < 160; x++)
    {
        f.shades[pixels[x] & 3] = true;
    }
}

// Every game must boot and draw something other than a blank screen within a minute of
// pressing Start and A now and then; this is what keeps a broken ROM out of the pack
static void testRoms()
{
    std::ifstream list("library.json");
    std::string text((std::istreambuf_iterator<char>(list)), std::istreambuf_iterator<char>());
    size_t at = 0, games = 0;
    while ((at = text.find("\"slug\": \"", at)) != std::string::npos)
    {
        at += 9;
        std::string slug = text.substr(at, text.find('"', at) - at);
        std::ifstream file("roms/" + slug + ".gb", std::ios::binary);
        std::vector<uint8_t> rom((std::istreambuf_iterator<char>(file)), std::istreambuf_iterator<char>());
        static GameBoy gb;
        Frame frame;
        bool started = !rom.empty() && gb.start(rom.data(), sink, &frame);
        int varied = 0;
        for (int i = 0; started && i < 3600 && !gb.crashed(); i++)
        {
            gb.setButtons((i % 120 > 100 ? JOYPAD_START : 0) | (i % 90 > 80 ? JOYPAD_A : 0));
            frame = Frame();
            gb.runFrame();
            gb.sound().frame();
            varied += (frame.shades[0] + frame.shades[1] + frame.shades[2] + frame.shades[3]) > 1;
        }
        if (!started || gb.crashed() || varied < 600)
        {
            printf("FAIL rom %s: started %d, crashed %d, %d varied frames\n", slug.c_str(), started,
                   started && gb.crashed(), varied);
            failures++;
        }
        gb.stop();
        games++;
    }
    CHECK(games > 0);
    printf("%zu ROMs booted\n", games);
}

int main()
{
    testPack();
    testCodec();
    testSaves();
    testScale();
    testApu();
    testRoms();
    if (failures > 0)
    {
        printf("%d failed\n", failures);
        return 1;
    }
    printf("All passed\n");
    return 0;
}
