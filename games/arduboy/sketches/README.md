# The games

Each folder is one game's sketch as its author published it, with its licence, built by
`tools/sketches.py` (see `../CLAUDE.md`). Only the source files were kept: the `.hex` builds,
images and artwork sources are gone. Every change made for the board is listed below; anything
not listed is upstream's.

| Folder | Game | Upstream | Commit | Licence |
|---|---|---|---|---|
| `nineteen43` | 1943 | https://github.com/ArduboyCollection/Nineteen43 | bf77065 (2017-10-09) | MIT |
| `choplifter` | Choplifter | https://github.com/ArduboyCollection/Choplifter | 6e875e5 (2018-02-08) | BSD-3-Clause |
| `kong` | Kong | https://github.com/Press-Play-On-Tape/Kong | ec95141 (2022-05-11) | BSD-3-Clause |
| `darkunder` | Dark & Under | https://github.com/ArduboyCollection/Dark-And-Under | 44b209e (2018-02-01) | BSD-3-Clause |
| `hellocommander` | Hello, Commander | https://github.com/ArduboyCollection/HelloCommander | f540d0a (2017-12-27) | MIT |
| `omegachase` | Omega Chase | https://github.com/Karl-Williams/OmegaChase | 64bc7d3 (2023-05-23) | MIT |
| `nightraid` | Night Raid | https://github.com/ArduboyCollection/night-raid | d74fcf6 (2018-02-12) | MIT |
| `cosmicpods` | Cosmic Pods | https://github.com/ArduboyCollection/CosmicPods | 0923919 (2017-08-29) | BSD-3-Clause |
| `randocity` | Randocity | https://github.com/pmwasson/Randocity | 42e9b7e (2019-12-05) | MIT |
| `flappyball` | Flappy Ball | https://github.com/ArduboyCollection/FlappyBall | 22253a6 (2018-01-26) | BSD-3-Clause (in `FlappyBall.ino`) |
| `crateconfusion` | Crate Confusion | https://github.com/phoboslab/arduboy-games | de5e858 (2019-03-15) | MIT (the repo's README; its LICENSE names OBONO, copied from another repo) |
| `waternet` | Waternet | https://github.com/joyrider3774/waternet_arduboy | 2f5e8ce (2022-06-14) | MIT |
| `pipes` | Pipes | https://github.com/ArduboyCollection/LayingPipe | d3c0279 (2018-02-13) | BSD-3-Clause |
| `ardulo` | ArduLO | https://github.com/jonthysell/ArduLO | 34d3b87 (2021-06-01) | MIT |
| `dominoes` | Dominoes | https://github.com/Press-Play-On-Tape/Dominoes | c094998 (2022-05-11) | BSD-3-Clause |

## Changes

- **choplifter**, **darkunder**: the trimmed copy of the Arduboy2 library each bundled
  (`src/Arduboy2/`, `src/Arduboy/`) is now a header that includes the board's port.
- **darkunder**: `Level::getMapElement` takes a square past the map's edge for a wall, rather than
  reading past the level's data (the view looks three squares ahead, beyond the border).
- **hellocommander**: the random seed comes from `generateRandomSeed()` rather than the AVR's
  ADC registers; `::tiles` in `world.cpp` names `ab_hellocommander::tiles`, since the game's
  globals live in that namespace; `stateVars` is aligned and two bytes longer, and the party
  screen keeps its 16-bit scroll position at byte 4 instead of 1, since the Xtensa faults on an
  unaligned 16-bit load; the cursor clears a tile's highlight only when the tile is on the map.
- **kong**: `src/arduboyTones/`, a copy of ArduboyTones driving timer 3, is a subclass of the
  board's ArduboyTones, with the jump's pitch slide played as one note; `../Utils/` includes
  are `../utils/` (the folder's real case); `Barrel::getRect`, `Gorilla::getStance` and
  `PlayGameState::drawScenery` return a value where they fell off the end.
- **omegachase**: an enemy aiming almost straight up picks direction 0 rather than reading a
  sixteenth one past the table.
- **nightraid**: `nextScoreForModulo` returns a value where it fell off the end.
- **pipes**: `initEEPROM` returns a value where it fell off the end; loading an odd-sized puzzle
  no longer clears the cell past each row, which on the last row of a 9x9 was past the board.
- **waternet**: `waternet.ino` includes with `/` rather than `\`; `level.cpp` checks a cell is on
  the board before reading it, not after.
