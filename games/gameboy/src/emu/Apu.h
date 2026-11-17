#ifndef APU_H
#define APU_H

#include <stdint.h>

// The Game Boy's sound registers, boiled down to the one note a passive buzzer can play: each
// frame, the loudest of the two square channels and the wave channel, as a frequency. The noise
// channel (drums) is left out. Envelopes and length counters run at frame steps, which is
// close enough for a buzzer.
class Apu
{
public:
    void reset();
    uint8_t read(uint16_t addr) const;
    void write(uint16_t addr, uint8_t value);

    // Advances envelopes and lengths by one frame; returns the note to play in Hz, 0 for silence
    uint16_t frame();

private:
    struct Channel
    {
        bool on = false;
        uint8_t volume = 0;     // 0..15
        uint8_t envStep = 0;    // Frames per envelope step, 0 for none
        bool envUp = false;
        uint8_t envCount = 0;
        uint16_t length = 0;    // Frames left while lengthOn
        bool lengthOn = false;
    };

    uint8_t regs[0x30] = {};
    Channel ch[3];

    uint16_t period(uint8_t lowReg) const;
    void trigger(uint8_t index);
};

#endif // APU_H
