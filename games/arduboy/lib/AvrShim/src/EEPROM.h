#ifndef AB_EEPROM_H
#define AB_EEPROM_H

#include <Arduino.h>
#include <string.h>

// The Arduboy's 1 KB EEPROM, one per game: the host loads the running game's copy from NVS
// before setup() and stores it once the game stops writing (src/play/Host.cpp)
static const int AB_EEPROM_SIZE = 1024;
extern uint8_t abEeprom[AB_EEPROM_SIZE];
// Counts writes that changed a byte; the host saves when it moved and then stood still
extern volatile uint32_t abEepromWrites;

struct EERef
{
    int index;
    EERef(int index) : index(index) {}
    uint8_t operator*() const { return abEeprom[index & (AB_EEPROM_SIZE - 1)]; }
    operator uint8_t() const { return **this; }
    EERef &operator=(uint8_t in)
    {
        uint8_t &cell = abEeprom[index & (AB_EEPROM_SIZE - 1)];
        if (cell != in)
        {
            cell = in;
            abEepromWrites++;
        }
        return *this;
    }
    EERef &operator=(const EERef &ref) { return *this = (uint8_t)ref; }
    EERef &update(uint8_t in) { return *this = in; }
    EERef &operator+=(uint8_t in) { return *this = **this + in; }
    EERef &operator-=(uint8_t in) { return *this = **this - in; }
    EERef &operator*=(uint8_t in) { return *this = **this * in; }
    EERef &operator/=(uint8_t in) { return *this = **this / in; }
    EERef &operator^=(uint8_t in) { return *this = **this ^ in; }
    EERef &operator%=(uint8_t in) { return *this = **this % in; }
    EERef &operator&=(uint8_t in) { return *this = **this & in; }
    EERef &operator|=(uint8_t in) { return *this = **this | in; }
    EERef &operator<<=(uint8_t in) { return *this = **this << in; }
    EERef &operator>>=(uint8_t in) { return *this = **this >> in; }
    EERef &operator++() { return *this = **this + 1; }
    EERef &operator--() { return *this = **this - 1; }
    uint8_t operator++(int)
    {
        uint8_t ret = **this;
        ++(*this);
        return ret;
    }
    uint8_t operator--(int)
    {
        uint8_t ret = **this;
        --(*this);
        return ret;
    }
};

struct EEPROMClass
{
    void begin(size_t = 0) {}
    bool commit() { return true; }
    void end() {}
    EERef operator[](int idx) { return EERef(idx); }
    uint8_t read(int idx) { return EERef{idx}; }
    void write(int idx, uint8_t val) { EERef{idx} = val; }
    void update(int idx, uint8_t val) { EERef{idx} = val; }
    uint16_t length() { return AB_EEPROM_SIZE; }

    template <typename T> T &get(int idx, T &t)
    {
        uint8_t *ptr = (uint8_t *)&t;
        for (size_t i = 0; i < sizeof(T); i++)
        {
            ptr[i] = read(idx + i);
        }
        return t;
    }

    template <typename T> const T &put(int idx, const T &t)
    {
        const uint8_t *ptr = (const uint8_t *)&t;
        for (size_t i = 0; i < sizeof(T); i++)
        {
            update(idx + i, ptr[i]);
        }
        return t;
    }
};

static EEPROMClass EEPROM;

#endif
