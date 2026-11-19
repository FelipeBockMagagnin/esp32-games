# CLAUDE.md

This file provides guidance to Claude Code (claude.ai/code) when working with code in this repository.

## Project

Crisp Games: 244 of ABA Games' one-button arcade minigames, written for crisp-game-lib, on an ESP32 with a 240x320 ILI9341 SPI display, five push buttons and a passive buzzer (the same board as `games/idle`). PlatformIO + Arduino framework. The games are C (`carts/`), built against crisp-game-lib-portable (`lib/cglp`); the firmware around them is C++ (`src/`), its screens and settings taken from `games/arduboy`.

## Code style

**Comment sparingly.** Write a comment only when the code cannot say it: a non-obvious constraint, a hardware quirk, or why something that looks wrong is deliberate. Do not narrate what the next line does, restate a name, label sections, or explain a change in a comment (that goes in the commit). A game in `carts/` keeps its author's style; change it only where it cannot run on the board, and list the change in `carts/README.md`. A change to `lib/cglp` goes in its `README.md`.

## Commands

`pio` is not on `PATH`; it lives at `~/.platformio/penv/bin/pio`.

```bash
pio run                              # Build
pio run -t upload -t monitor         # Flash and open the serial monitor (115200)
pio run -e dev -t upload -t monitor  # Dev build: serial commands screen <i>, play <i>; fps every 5 s
tools/run.sh <slug> out.png [keys:ms]...   # Play a game on the PC under AddressSanitizer, save the panel
tools/run.sh list|settings out.png [theme] # The screens around the games
```

`tools/run.sh` builds the real library, host and games against `tools/stub/` with ASan and UBSan, on a virtual clock, pressing the buttons its script names (`tools/run.cpp`: U D L R O, `_` for none, `?` for OK tapped and held at random). It ends with a line of frames played, games over and the PC's time a frame. **Run every game through it after touching `lib/` or adding one**; `CRISP_LIBRARY` and `CRISP_CARTS` point it at another catalogue, to try games before they join. `screen.png` is a Thunder scene drawn this way by `tools/make_mockups.py`.

## The games

- **`library.json`** is the catalogue, in list order (by title): `slug` (the file `carts/<slug>.c`), `title`, `author`, `by` (the strip's credit), `license`, `source` (the original JavaScript), `port` (who wrote the C, when not ABA Games), `controls` and `menu` (below). The site lists the games from it through `publish.py`.
- **`tools/carts.py`** (a PlatformIO pre-script) writes `CartTable.inc` from it, which `src/play/Carts.c` turns into the table of games and the library's `addGames()`, and builds each listed cart beside `Carts.c`. A file in `carts/` that `library.json` does not list stops the build.
- **Only games whose author published them under a licence that allows redistribution.** `carts/README.md` records where they came from, the commit, which were left out and why, and every change made to them.
- **Adding a game:** copy its `game<Name>.c` into `carts/<slug>.c`, make its `characters` `const`, add it to `library.json`, build, and play it through `tools/run.sh` (`?:90000` at least). What to look for: `memcpy()` over itself, an index one past an array, a `bool` read before it was written, and its `.bss` (`xtensa-esp32-elf-size` of its object): the largest game's is the RAM every game gets.

## Controls

No chords. Each game's `controls` says what reaches it:
- `tap` (the default, most games): only OK, as the library's A, so every direction is free.
- `arrows`: the directions as well, but for the one that opens the menu.
- `slide`: for games that follow a finger along x (`input.pos.x`), Left and Right move a pointer the host marks with a caret on the bottom edge, faster the longer they are held; OK taps.

The menu is a press of `menu` (Up unless the entry says otherwise). It draws over the game: Resume, Restart game, Quit to list, Game menu (when the launcher is installed). Left also resumes. A game that points in two dimensions, or needs all four directions, does not fit.

## Hardware

- **All TFT_eSPI setup lives in the repo root's `common.ini` (`[board]`)**. Portrait, as most games.
- **Backlight** (`TFT_BL`, 21) is PWM on LEDC channel 4 (`Backlight` in `lib/dotcore`), since the buzzer owns channel 0. It never dims during a game.
- Hold Up + Down while powering on to erase every score; Settings also has a reset. Hold Confirm to open the game menu (`returnToLauncher()`); Settings and the pause menu have a Game menu row when the launcher is installed.

## Architecture

- **`src/play/Crisp.c`** is the only C++-facing door into the library (`Crisp.h`): `cglp.h` defines macros such as `SELECT`, `RED` and `CLICK` and functions named `text()` and `line()`, which collide with the firmware's names.
- **`src/play/Host`** is what `machineDependent.h` asks for. The library draws into a frame of palette indices at the game's own size (at most 200x100, 1 byte a pixel; each RGB it passes becomes an index). It goes out at 2x when that fits the panel's 240 width (100x100 is 200x200), otherwise stretched to 240 (200x100 is 240x120), in bands of 16 rows by DMA, two buffers in turn. A band whose source rows hash the same as last frame is not sent. The game runs at 60 steps a second by `micros()`; when the push falls behind, a frame is not shown (never four in a row), but the game still steps. The strip below names the game, its keys and its best score.
- **Sound:** the library schedules notes ahead (`md_playTone(freq, duration, when)` on `md_getAudioTime()`'s clock); an `esp_timer` every 2 ms plays the latest note due, as the library's M5Stack port does, through `SoundManager` and so `Volume.h`.
- **RAM:** the games' zero-initialised statics are laid over each other. `tools/carts.py` renames each cart's `.bss` to `.crisp.<slug>` (built with `-fno-data-sections -fno-common`) and writes an `OVERLAY` that puts them all at one address after the firmware's `.bss`. `tools/overlay.py` (a post-script) hands that script to ld between `memory.ld` and `sections.ld`. The host zeroes `crisp_overlay_start`..`_end` before a game starts or restarts. The library's big tables and the host's frame and bands come from the heap. The boot log prints the heap left.
- Quitting needs no restart: a game sets itself up again at `ticks == 0`.
- **Screens:** the game list (title over the game's own controls, the best score) ↔ Settings (sound: on, games only, off; the palette of the menus; dimming; game menu; reset). Holding Up or Down speeds up through the list.

### Colours
The games keep crisp-game-lib's colours. **Every other colour comes from `src/ui/Theme.h`** as a `Color` slot, the ones `lib/dotui` draws with; a theme dresses the menus, the frame and the strip.

### Persistence
NVS namespace `crisp`: every best score in one record (`scores`, an id, the FNV-1a of the slug, and a score per game that has one), written when a game over beats it; the settings under their own key.
