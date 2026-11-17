#pragma once
// avr-libc's EEPROM calls, on the same emulated EEPROM as EEPROM.h
#include "../EEPROM.h"

static inline uint8_t eeprom_read_byte(const uint8_t *addr) { return EEPROM.read((int)(uintptr_t)addr); }
static inline void eeprom_write_byte(uint8_t *addr, uint8_t value) { EEPROM.write((int)(uintptr_t)addr, value); }
static inline void eeprom_update_byte(uint8_t *addr, uint8_t value) { EEPROM.update((int)(uintptr_t)addr, value); }
static inline uint16_t eeprom_read_word(const uint16_t *addr)
{
    int a = (int)(uintptr_t)addr;
    return (uint16_t)(EEPROM.read(a) | EEPROM.read(a + 1) << 8);
}
static inline void eeprom_update_word(uint16_t *addr, uint16_t value)
{
    int a = (int)(uintptr_t)addr;
    EEPROM.update(a, (uint8_t)value);
    EEPROM.update(a + 1, (uint8_t)(value >> 8));
}
static inline void eeprom_write_word(uint16_t *addr, uint16_t value) { eeprom_update_word(addr, value); }
static inline uint32_t eeprom_read_dword(const uint32_t *addr)
{
    int a = (int)(uintptr_t)addr;
    return (uint32_t)EEPROM.read(a) | (uint32_t)EEPROM.read(a + 1) << 8 | (uint32_t)EEPROM.read(a + 2) << 16 |
           (uint32_t)EEPROM.read(a + 3) << 24;
}
static inline void eeprom_update_dword(uint32_t *addr, uint32_t value)
{
    int a = (int)(uintptr_t)addr;
    for (int i = 0; i < 4; i++)
    {
        EEPROM.update(a + i, (uint8_t)(value >> (8 * i)));
    }
}
static inline void eeprom_write_dword(uint32_t *addr, uint32_t value) { eeprom_update_dword(addr, value); }
static inline void eeprom_read_block(void *dst, const void *src, size_t n)
{
    for (size_t i = 0; i < n; i++)
    {
        ((uint8_t *)dst)[i] = EEPROM.read((int)(uintptr_t)src + i);
    }
}
static inline void eeprom_update_block(const void *src, void *dst, size_t n)
{
    for (size_t i = 0; i < n; i++)
    {
        EEPROM.update((int)(uintptr_t)dst + i, ((const uint8_t *)src)[i]);
    }
}
static inline void eeprom_write_block(const void *src, void *dst, size_t n) { eeprom_update_block(src, dst, n); }
