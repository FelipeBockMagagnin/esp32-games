#pragma once
// Flash and RAM share one address space on the ESP32: PROGMEM data is read in place
#include <pgmspace.h>
#include <stdint.h>
#include <type_traits>

// A pointer is 16 bits on the AVR, so games keep tables of pointers and read them with
// pgm_read_word(). Here that read returns the whole pointer when the table holds pointers, and
// otherwise the two bytes one at a time: the Xtensa faults on an unaligned 16-bit load, which the
// AVR games never had to care about.
namespace abpgm
{
    template <class T>
    inline typename std::enable_if<std::is_pointer<T>::value, T>::type word(const T *p)
    {
        return *p;
    }

    template <class T>
    inline typename std::enable_if<!std::is_pointer<T>::value, uint16_t>::type word(const T *p)
    {
        const uint8_t *b = (const uint8_t *)p;
        return (uint16_t)(b[0] | b[1] << 8);
    }

    template <class I>
    inline typename std::enable_if<std::is_integral<I>::value, uint16_t>::type word(I address)
    {
        return word((const uint8_t *)(uintptr_t)address);
    }

    inline uint32_t dword(const void *p)
    {
        const uint8_t *b = (const uint8_t *)p;
        return (uint32_t)b[0] | (uint32_t)b[1] << 8 | (uint32_t)b[2] << 16 | (uint32_t)b[3] << 24;
    }
}

#undef pgm_read_word
#undef pgm_read_word_near
#undef pgm_read_word_far
#undef pgm_read_dword
#undef pgm_read_dword_near
#undef pgm_read_dword_far
#define pgm_read_word(addr) (abpgm::word(addr))
#define pgm_read_word_near(addr) pgm_read_word(addr)
#define pgm_read_word_far(addr) pgm_read_word(addr)
#define pgm_read_dword(addr) (abpgm::dword((const void *)(addr)))
#define pgm_read_dword_near(addr) pgm_read_dword(addr)
#define pgm_read_dword_far(addr) pgm_read_dword(addr)
