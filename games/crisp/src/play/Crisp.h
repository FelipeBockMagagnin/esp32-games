#ifndef CRISP_H
#define CRISP_H

#include <stdbool.h>

// The library seen from C++. cglp.h defines macros as plain as SELECT, RED and CLICK and
// functions named text() and line(), which would collide with the firmware's own names, so
// only src/play/Crisp.c includes it.
#ifdef __cplusplus
extern "C" {
#endif

void crispInit(void);
int crispCount(void);
const char *crispTitle(int game);
const char *crispDescription(int game);
int crispViewW(int game);
int crispViewH(int game);
void crispStart(int game);
// One 1/60 s step. `px`, `py` is the pointer, for the games that follow a finger
void crispFrame(bool left, bool right, bool up, bool down, bool a, float px, float py);
int crispHiScore(int game);
void crispSetHiScore(int game, int score);

#ifdef __cplusplus
}
#endif

#endif // CRISP_H
