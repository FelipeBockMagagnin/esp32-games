#ifndef SAVE_CODEC_H
#define SAVE_CODEC_H

#include <stddef.h>
#include <stdint.h>

// PackBits, for cartridge RAM: a homebrew save is mostly one repeated byte, so its 8 or 32 KB
// come down to a few hundred bytes. A header byte n in 0..127 copies the next n + 1 bytes;
// 129..255 repeats the next byte 257 - n times.

// The packed length, or 0 when it would need more than `capacity` bytes
size_t packSave(const uint8_t *in, size_t length, uint8_t *out, size_t capacity);

// False unless the packed data fills exactly `length` bytes. With no `out`, only checks that.
bool unpackSave(const uint8_t *in, size_t packed, uint8_t *out, size_t length);

#endif // SAVE_CODEC_H
