// PROGMEM as the ESP32 core has it: flash is read in place
#ifndef STUB_PGMSPACE_H
#define STUB_PGMSPACE_H

#include <cstdint>
#include <cstring>

#define PROGMEM
#define PGM_P const char *
#define PSTR(s) (s)
#define pgm_read_byte(addr) (*(const unsigned char *)(addr))
#define pgm_read_word(addr) (*(const unsigned short *)(addr))
#define pgm_read_dword(addr) (*(const unsigned long *)(addr))
#define pgm_read_float(addr) (*(const float *)(addr))
#define pgm_read_ptr(addr) (*(void *const *)(addr))
#define pgm_read_byte_near(addr) pgm_read_byte(addr)
#define pgm_read_word_near(addr) pgm_read_word(addr)
#define pgm_read_dword_near(addr) pgm_read_dword(addr)
#define pgm_read_ptr_near(addr) pgm_read_ptr(addr)
#define memcpy_P memcpy
#define memcmp_P memcmp
#define strcpy_P strcpy
#define strncpy_P strncpy
#define strcat_P strcat
#define strcmp_P strcmp
#define strlen_P strlen
#define sprintf_P sprintf
#define snprintf_P snprintf
#define printf_P printf

#endif
