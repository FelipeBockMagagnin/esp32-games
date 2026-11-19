#include "menu.h"

#include "cglp.h"


int gameCount = 0;
static Game games[MAX_GAME_COUNT];

void addGame(char *title, char *description,
             const char (*characters)[CHARACTER_WIDTH][CHARACTER_HEIGHT + 1],
             int charactersCount, Options options, int usesMouse, void (*update)(void)) {
  if (gameCount >= MAX_GAME_COUNT) {
    return;
  }
  Game *g = &games[gameCount];
  g->title = title;
  g->description = description;
  g->characters = characters;
  g->charactersCount = charactersCount;
  g->options = options;
  g->usesMouse = usesMouse;
  g->update = update;
  gameCount++;
}

Game getGame(int index) { return games[index]; }

