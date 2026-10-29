# CLAUDE.md

This file provides guidance to Claude Code (claude.ai/code) when working with code in this repository.

## Project

Campo Minado, the minesweeper genre, for an ESP32 with a 240x320 ILI9341 SPI display, five push buttons and a passive buzzer (the same board as `games/idle`). PlatformIO + Arduino framework, C++. Its engine (core, settings, saves) came from `games/snake`, the key repeat from `games/dice`. Three levels whose tiles grow so each fills the screen below the HUD (Easy 9x12 at 24px, Normal 12x16 at 18px, Hard 14x18 at 16px), a best time and wins per level, eight themes.

## Code style

**Comment sparingly.** Write a comment only when the code cannot say it: a non-obvious constraint, a hardware quirk, or why something that looks wrong is deliberate. Do not narrate what the next line does, restate a name, label sections, or explain a change in a comment (that goes in the commit).

## Commands

`pio` is not on `PATH`; it lives at `~/.platformio/penv/bin/pio`.

```bash
pio run                              # Build
pio run -t upload -t monitor         # Flash and open the serial monitor (115200)
pio run -e dev -t upload -t monitor  # Dev build: serial commands screen <i>, shot
~/.platformio/penv/bin/python tools/screenshot.py out.png [--screen 1]  # Dev build on the board: the display as a PNG
tools/test.sh                        # Logic tests: the safe first open, counts, flood waves, chords, flags, winning, the save
tools/shots.sh [out_dir]             # The real screens drawn on the PC, one PNG per scripted scene, every theme
```

`tools/test.sh` compiles `src/mines/` and `lib/dotcore`'s `SlotStore.cpp` against `tools/stub/`. **Rerun it after touching `src/mines/`.** `tools/shots.sh` builds the screens against `tools/stub/TFT_eSPI.h` (fonts from `.pio/libdeps`, so it builds the firmware once if that is missing) and plays them by script; `tools/make_mockups.py` takes the site's `screen.png` from its `06_playing` scene.

## Hardware

- **All TFT_eSPI setup lives in the repo root's `common.ini` (`[board]`)**; only the GLCD font, so the tile digits are bitmaps in `TileArt.cpp`.
- **Pins are `const int` at the top of `src/main.cpp`**: right 23, left 26, confirm 27, up 19, down 22, buzzer 32.
- **Backlight** (`TFT_BL`, 21) is PWM on LEDC channel 4 (`Backlight` in `lib/dotcore`), since `tone()` owns channel 0.
- Hold Up + Down while powering on to erase the save; Settings also has a reset.
- **The game menu:** when the site installed the launcher, Settings has a Game menu row (`returnToLauncher()`), and holding Confirm while powering on opens it.

## Controls

Arrows move the cursor (held, they repeat); Left/Right wrap round the row. Up past the top row, or Down past the bottom one, lands on the pause button in the HUD. A **tap** of Confirm (released before 400 ms) opens the tile, or on a number whose flags match it opens its neighbours; **holding** it plants or lifts a flag, a ring of dots filling round the cursor from 120 ms. The press only starts the hold: `PlayScreen` reads the button's level through the `confirmDown` it is given.

## Architecture

**No blocking `delay()`.** Elapsed time uses unsigned subtraction (`now - last`), deadlines a signed difference.

- **`src/mines/`** is plain C++ with no Arduino or TFT include. `Field` is up to 14x18 cells, a byte each (count, mine, open, flag). The mines are laid on the first open, never on that cell or its eight neighbours, so the first open always floods. Every open and chord lists the cells it opened in flood order with each one's wave (its step in the flood); the renderer reveals them from that list. Winning flags the mines that were left. `LEVELS` holds each level's size, mines and tile size. `Records` (level, best time in tenths, wins and plays per level) goes through `SlotStore`; adding a field means bumping `RECORDS_VERSION`.
- **`src/ui/TileArt`** draws one tile, `s` pixels a side, in the active theme's `TileStyle` (bevel, flat, glow, sketch, grass, wave) and `MineGlyph` (bomb, flower, urchin, gem). Every tile paints its whole square, so it never needs erasing; the cursor, the hold ring and the sparkle are drawn over one and cleared by drawing it again.
- **`src/ui/FieldView`** draws a `Field` at an origin and runs its animations a frame per call: the reveal (a cell of wave `w` sinks open over `POP_FRAMES` frames from frame `w`, so the flood spreads ring by ring), the win sweep across the diagonals (flags rise over the mines, the rest glints) and the covering wave. The title's demo and the game share it.
- **Screens:** a loop of Title (a self-playing demo field, Play, Level, Theme, each level's best time and wins) ↔ Settings (sound, dimming, game menu, reset). `PlayScreen` is a modal opened from the title and takes every arrow. It draws the field straight to the panel from `update()`; only the mine counter and the clock are elements. A blast flashes the mine, then the other mines turn up nearest first and the wrong flags are crossed; a win sweeps; both end in a box (Again / Menu). The pause box offers Resume, Restart, Quit. The clock starts at the first open, which also counts the game as played.
- **Sound** (`SoundManager.h`): a note per flood wave climbing a pentatonic scale, a note per lone number, flag up/down pairs, the blast's falling run and a knock per chained mine, the win fanfare and the record jingle. Quiet keeps only flags, blasts, wins and records.

### Colours
**Every colour comes from `src/ui/Theme.h`** as a `Color` slot: the ones `lib/dotui` draws with, then the field's (`FIELD`, `COVER` and its two edges, `OPEN` and its grid line, `NUM1`-`NUM8`, `MINE`, `MINE_HI`, `FLAG`, `CURSOR`, `BLAST`). The flag's pole is `NUM7`. Each `ThemeDef` in `Themes.h` also names its tile style and mine glyph.

### Persistence
NVS namespace `mines`. Records are written when the level changes, when a game starts and when one is won; settings live under their own key.
