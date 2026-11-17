#include "GameBoy.h"
#include <setjmp.h>
#include <stdlib.h>
#include <string.h>

static const uint16_t CART_TYPE = 0x0147;
// Cartridge types with a battery, so their RAM outlives power-off and is worth a save
static const uint8_t BATTERY_TYPES[] = {0x03, 0x06, 0x09, 0x0D, 0x0F, 0x10, 0x13, 0x1B, 0x1E};

static Apu *activeApu = nullptr;
// Peanut-GB must not return from gb_error (it marks the call unreachable), so the error jumps
// back out of gb_run_frame()
static jmp_buf crashJump;

extern "C" uint8_t audio_read(uint16_t addr)
{
    return activeApu != nullptr ? activeApu->read(addr) : 0xFF;
}

extern "C" void audio_write(uint16_t addr, uint8_t val)
{
    if (activeApu != nullptr)
    {
        activeApu->write(addr, val);
    }
}

static GameBoy &self(struct gb_s *gb)
{
    return *(GameBoy *)gb->direct.priv;
}

uint8_t GameBoy::romRead(struct gb_s *gb, const uint_fast32_t addr)
{
    return self(gb).rom[addr];
}

uint8_t GameBoy::ramRead(struct gb_s *gb, const uint_fast32_t addr)
{
    GameBoy &g = self(gb);
    return addr < g.ramSize ? g.ram[addr] : 0xFF;
}

void GameBoy::ramWrite(struct gb_s *gb, const uint_fast32_t addr, const uint8_t value)
{
    GameBoy &g = self(gb);
    if (addr < g.ramSize && g.ram[addr] != value)
    {
        g.ram[addr] = value;
        g.changed = true;
        g.framesSinceWrite = 0;
    }
}

void GameBoy::onError(struct gb_s *gb, const enum gb_error_e, const uint16_t)
{
    self(gb).error = true;
    longjmp(crashJump, 1);
}

void GameBoy::drawLine(struct gb_s *gb, const uint8_t *pixels, const uint_fast8_t line)
{
    GameBoy &g = self(gb);
    g.sink(g.context, pixels, (uint8_t)line);
}

bool GameBoy::start(const uint8_t *romData, LineSink lineSink, void *sinkContext)
{
    stop();
    rom = romData;
    sink = lineSink;
    context = sinkContext;
    error = false;
    if (gb_init(&gb, romRead, ramRead, ramWrite, onError, this) != GB_INIT_NO_ERROR)
    {
        return false;
    }
    size_t size = 0;
    if (gb_get_save_size_s(&gb, &size) != 0)
    {
        return false;
    }
    if (size > 0)
    {
        ram = (uint8_t *)malloc(size);
        if (ram == nullptr)
        {
            return false;
        }
        memset(ram, 0xFF, size);
    }
    ramSize = size;
    battery = memchr(BATTERY_TYPES, rom[CART_TYPE], sizeof(BATTERY_TYPES)) != nullptr;
    gb_init_lcd(&gb, drawLine);
    apu.reset();
    activeApu = &apu;
    changed = false;
    framesSinceWrite = 0;
    started = true;
    return true;
}

void GameBoy::stop()
{
    free(ram);
    ram = nullptr;
    ramSize = 0;
    battery = false;
    started = false;
    if (activeApu == &apu)
    {
        activeApu = nullptr;
    }
}

void GameBoy::runFrame()
{
    if (!started || error)
    {
        return;
    }
    if (setjmp(crashJump) == 0)
    {
        gb_run_frame(&gb);
    }
    framesSinceWrite++;
}
