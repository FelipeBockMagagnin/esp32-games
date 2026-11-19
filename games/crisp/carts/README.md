# The games

Every game here is by Kenta Cho (ABA Games), who wrote them in JavaScript for crisp-game-lib and published them under the MIT licence (`LICENSE` in `../lib/cglp/`, ABA Games 2022). `library.json`'s `source` points at each game's original:

| Repository | Games |
|---|---|
| [abagames/crisp-game-lib](https://github.com/abagames/crisp-game-lib) (`docs/`) | 83 |
| [abagames/claude-one-button-game-creation](https://github.com/abagames/claude-one-button-game-creation), games he made with an AI chatbot | 80 |
| [abagames/crisp-game-lib-games](https://github.com/abagames/crisp-game-lib-games) | 59 |
| [abagames/crisp-game-lib-11-games](https://github.com/abagames/crisp-game-lib-11-games) | 22 |

The C is not ours either. Fifteen games (those without `port` in `library.json`) are ABA Games' own C, from [crisp-game-lib-portable](https://github.com/abagames/crisp-game-lib-portable). The rest were ported to C by Willems Davy (joyrider3774), many with an AI's help, in [crisp-game-lib-portable-sdl](https://github.com/joyrider3774/crisp-game-lib-portable-sdl), MIT. Every file here comes from that repository's `src/games/game<Name>.c` at commit `04bc2c4cfc2853628eda25a46190bb7ac6fe216a` (2026-08-02), renamed to its slug.

## Left out

Of that repository's 266 games, 22 are not here:

- they point in two dimensions, which five buttons cannot do (`input.pos.y`): B Blast, Balloon, Breed C, C Nodes, Cross Line, Hyper Laser, Make Maze, Marusansi, Pairs Drop, Regene, Sky Golf, Smoke G, VH Walls;
- they need all four directions and OK, leaving no button for the menu: Ring Blast, T Laser;
- L Rain needs 139 KB of RAM;
- Cleaner and Fracave run three times slower a frame than the rest;
- Mole N reads past the end of its grid;
- Magnetic Pendulum, Ricochet Pins and Windrang appear in none of ABA Games' repositories, so their licence cannot be checked.

## What changed

- **Every game:** `static char characters[]...` is `static const char characters[]...`, so its art stays in flash rather than RAM.
- **`memcpy()` to `memmove()`** where a game shifts an array onto itself, which `memcpy()` does not allow: Geocent, G Tail, I Slash, Number Ball, P Fit, Snake 1, Square Bar, Tape J.
- **Ladder Drop:** a panel's ladder loop read `lxs[li]` one past the last ladder; it now checks `li < lxsCount` first.
