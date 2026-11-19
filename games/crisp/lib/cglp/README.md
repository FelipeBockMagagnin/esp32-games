# cglp

crisp-game-lib-portable, ABA Games' C version of crisp-game-lib (MIT, `LICENSE`), as extended by joyrider3774 in [crisp-game-lib-portable-sdl](https://github.com/joyrider3774/crisp-game-lib-portable-sdl) (`src/lib/`, commit `04bc2c4`): more games in the list, best scores, `input.pos`, a spatial grid for collisions. The board's side of it (`machineDependent.h`) is `src/play/Host.cpp`.

What changed for the board:

- **RAM.** A hitbox's position and size are `int16_t`, not `float`. The collision grid is 16x16 cells of linked lists in one pool of 4096 entries, instead of 32x32 cells of 512 slots (2 MB). The hitboxes, the pool and the character cache come from `malloc()` in `initGame()`, since the ESP32's static RAM is 124 KB. `textPatterns` and the games' `characters` are `const`. `MAX_GAME_COUNT` is 256.
- **Best scores** are an `int` per game (`hiScores[]`, indexed like the game list), not a title of 100 characters to look up; the firmware stores them.
- **No menu.** `menu.c` keeps only the list of games; the firmware has its own, and starts a game with `restartGame()`. `initGame()` only registers the games. The chord that went to the menu or muted the sound (Up + Down with A or B) is gone.
- **Bugs:** a character drawn `TRANSPARENT` read `colorRgbs[-1]` while building its hitbox (Paku Paku, Ball Tour, Ladder Drop and others do this); a `Collision` was cleared only over the printable characters, though some games read all of it.
- **Seeds:** the ESP32 has no clock at boot, so games are seeded from `esp_random()`; the PC harness (`CRISP_PC`) seeds them in sequence, so a run plays the same every time.
- `-fwrapv` (`platformio.ini`): the sound's hash relies on a signed int wrapping.
