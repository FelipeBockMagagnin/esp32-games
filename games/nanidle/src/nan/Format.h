#ifndef NAN_FORMAT_H
#define NAN_FORMAT_H

#include <stddef.h>
#include <stdint.h>

// Every number on screen goes through these. Values grow without bound, so past the named
// suffixes they turn to scientific notation.
//   formatNumber: "999", "12.3K", "4.56M", "1.23Qa", "1.2e48" (7 chars max)
//   formatCompact: the same in 5 chars, for the damage numbers over a sprite
//   formatTime: "45s", "12m 05s", "3h 20m", "2d 04h"
void formatNumber(double value, char *out, size_t size);
void formatCompact(double value, char *out, size_t size);
void formatTime(double seconds, char *out, size_t size);

#endif // NAN_FORMAT_H
