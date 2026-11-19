#include "Crisp.h"
#include "cglp.h"

void crispInit(void) { initGame(); }

int crispCount(void) { return gameCount; }

const char *crispTitle(int game) { return getGame(game).title; }

const char *crispDescription(int game) { return getGame(game).description; }

int crispViewW(int game) { return getGame(game).options.viewSizeX; }

int crispViewH(int game) { return getGame(game).options.viewSizeY; }

void crispStart(int game) { restartGame(game); }

void crispFrame(bool left, bool right, bool up, bool down, bool a, float px, float py) {
  setButtonState(left, right, up, down, false, a);
  setMousePos(px, py);
  updateFrame();
}

int crispHiScore(int game) { return hiScores[game]; }

void crispSetHiScore(int game, int score) { hiScores[game] = score; }
