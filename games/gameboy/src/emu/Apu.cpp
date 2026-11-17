#include "Apu.h"
#include <string.h>

static const uint16_t BASE = 0xFF10;
static const uint8_t NR52 = 0x16;
static const uint8_t NR51 = 0x15;

// Bits that read back as 1 whatever was written, from Pan Docs' register table
static const uint8_t READ_MASK[0x30] = {
    0x80, 0x3F, 0x00, 0xFF, 0xBF, 0xFF, 0x3F, 0x00, 0xFF, 0xBF, 0x7F, 0xFF, 0x9F, 0xFF, 0xBF, 0xFF,
    0xFF, 0x00, 0x00, 0xBF, 0x00, 0x00, 0x70, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF,
    0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00};

// Per channel: its NRx1 (length), NRx2 (envelope or wave volume) and NRx3 (period low) registers
static const uint8_t LENGTH_REG[3] = {0x01, 0x06, 0x0B};
static const uint8_t VOLUME_REG[3] = {0x02, 0x07, 0x0C};
static const uint8_t PERIOD_REG[3] = {0x03, 0x08, 0x0D};
// The wave channel's volume code, 0 to 3, as a square channel's 0..15
static const uint8_t WAVE_VOLUME[4] = {0, 15, 7, 3};

void Apu::reset()
{
    memset(regs, 0, sizeof(regs));
    for (Channel &c : ch)
    {
        c = Channel();
    }
}

uint8_t Apu::read(uint16_t addr) const
{
    uint8_t i = (uint8_t)(addr - BASE);
    if (i >= sizeof(regs))
    {
        return 0xFF;
    }
    if (i == NR52)
    {
        return (regs[NR52] & 0x80) | 0x70 | (ch[0].on ? 1 : 0) | (ch[1].on ? 2 : 0) | (ch[2].on ? 4 : 0);
    }
    return regs[i] | READ_MASK[i];
}

uint16_t Apu::period(uint8_t lowReg) const
{
    return (uint16_t)(regs[lowReg] | (regs[lowReg + 1] & 7) << 8);
}

void Apu::trigger(uint8_t index)
{
    Channel &c = ch[index];
    uint8_t lengthReg = regs[LENGTH_REG[index]];
    if (index == 2)
    {
        c.on = (regs[0x0A] & 0x80) != 0;
        c.volume = WAVE_VOLUME[regs[0x0C] >> 5 & 3];
        c.envStep = 0;
        // 256 steps of 1/256 s, in frames
        c.length = (uint16_t)((256 - lengthReg) * 60 / 256 + 1);
    }
    else
    {
        uint8_t env = regs[VOLUME_REG[index]];
        c.volume = env >> 4;
        c.envUp = (env & 0x08) != 0;
        c.envStep = env & 7;
        c.envCount = 0;
        // A channel whose DAC is off (no volume, envelope going down) cannot sound
        c.on = (env & 0xF8) != 0;
        c.length = (uint16_t)((64 - (lengthReg & 0x3F)) * 60 / 256 + 1);
    }
    c.lengthOn = (regs[PERIOD_REG[index] + 1] & 0x40) != 0;
}

void Apu::write(uint16_t addr, uint8_t value)
{
    uint8_t i = (uint8_t)(addr - BASE);
    if (i >= sizeof(regs))
    {
        return;
    }
    if (i == NR52)
    {
        regs[NR52] = value & 0x80;
        if (!(value & 0x80))
        {
            for (Channel &c : ch)
            {
                c.on = false;
            }
        }
        return;
    }
    regs[i] = value;
    // A square channel's DAC is off with no volume and the envelope going down, and that
    // silences it at once
    if ((i == VOLUME_REG[0] || i == VOLUME_REG[1]) && !(value & 0xF8))
    {
        ch[i == VOLUME_REG[0] ? 0 : 1].on = false;
    }
    if (i == 0x0A && !(value & 0x80))
    {
        ch[2].on = false;
    }
    if (i == 0x0C)
    {
        ch[2].volume = WAVE_VOLUME[value >> 5 & 3];
    }
    for (uint8_t c = 0; c < 3; c++)
    {
        if (i == PERIOD_REG[c] + 1 && (value & 0x80))
        {
            trigger(c);
        }
    }
}

uint16_t Apu::frame()
{
    if (!(regs[NR52] & 0x80))
    {
        return 0;
    }
    for (uint8_t i = 0; i < 3; i++)
    {
        Channel &c = ch[i];
        if (!c.on)
        {
            continue;
        }
        if (c.lengthOn && c.length > 0 && --c.length == 0)
        {
            c.on = false;
        }
        // An envelope step is n/64 s: very near one frame per n
        if (c.envStep != 0 && ++c.envCount >= c.envStep)
        {
            c.envCount = 0;
            if (c.envUp && c.volume < 15)
            {
                c.volume++;
            }
            else if (!c.envUp && c.volume > 0)
            {
                c.volume--;
            }
        }
    }

    int best = -1;
    uint8_t loudest = 0;
    for (uint8_t i = 0; i < 3; i++)
    {
        // Panned to neither speaker, the channel is mute
        bool panned = (regs[NR51] & (0x11 << i)) != 0;
        if (ch[i].on && panned && ch[i].volume > loudest)
        {
            loudest = ch[i].volume;
            best = i;
        }
    }
    if (best < 0)
    {
        return 0;
    }
    uint16_t p = period(PERIOD_REG[best]);
    if (p >= 2048)
    {
        return 0;
    }
    uint32_t hz = (best == 2 ? 65536u : 131072u) / (2048u - p);
    // Beyond what the buzzer plays as a note: high squeals and sub-bass rumble
    return (hz < 40 || hz > 8000) ? 0 : (uint16_t)hz;
}
