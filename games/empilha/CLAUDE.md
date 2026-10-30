# CLAUDE.md

This file provides guidance to Claude Code (claude.ai/code) when working with code in this repository.

## Project

Empilha, the falling-blocks genre, for an ESP32 with a 240x320 ILI9341 SPI display, five push buttons and a passive buzzer (the same board as `games/idle`). PlatformIO + Arduino framework, C++. Its engine (core, settings, saves) came from `games/mines`. A well 11 cells wide and 21 tall at 15px, three modes (Endless, Sprint, Cascade), a best run per mode, six themes.

**It is an original game in the genre, never a copy of one.** The owners of the best-known game of this kind take clones down, so the name, the art, the sounds and the rules stay our own: never use that name, its music, its piece colours, its 10x20 well or its scoring table.

## Code style

**Comment sparingly.** Write a comment only when the code cannot say it: a non-obvious constraint, a hardware quirk, or why something that looks wrong is deliberate. Do not narrate what the next line does, restate a name, label sections, or explain a change in a comment (that goes in the commit).

## Commands

`pio` is not on `PATH`; it lives at `~/.platformio/penv/bin/pio`.

```bash
pio run                              # Build
pio run -t upload -t monitor         # Flash and open the serial monitor (115200)
pio run -e dev -t upload -t monitor  # Dev build: serial commands screen <i>, shot
~/.platformio/penv/bin/python tools/screenshot.py out.png [--screen 1]  # Dev build on the board: the display as a PNG
tools/test.sh                        # Logic tests: pieces, bag, moves, locking, clears, scores, the cascade, Sprint, the save
tools/shots.sh [out_dir]             # The real screens drawn on the PC, one PNG per scripted scene, every theme
```

`tools/test.sh` compiles `src/stack/` and `lib/dotcore`'s `SlotStore.cpp` against `tools/stub/`. **Rerun it after touching `src/stack/`.** `tools/shots.sh` builds the screens against `tools/stub/TFT_eSPI.h` (fonts from `.pio/libdeps`, so it builds the firmware once if that is missing) and plays them with a small bot that presses the real keys; `tools/make_mockups.py` takes the site's `screen.png` from its `09_cascade_set_up` scene.

## Hardware

- **All TFT_eSPI setup lives in the repo root's `common.ini` (`[board]`)**; only the GLCD font.
- **Pins are `const int` at the top of `src/main.cpp`**: right 23, left 26, confirm 27, up 19, down 22, buzzer 32.
- **Buttons debounce at 15 ms**, not the default 50, as an action game must answer at once.
- **Backlight** (`TFT_BL`, 21) is PWM on LEDC channel 4 (`Backlight` in `lib/dotcore`), since `tone()` owns channel 0.
- Hold Up + Down while powering on to erase the save; Settings also has a reset.
- **The game menu:** when the site installed the launcher, Settings has a Game menu row (`returnToLauncher()`), and holding Confirm while powering on opens it.

## Controls

Left/Right slide the piece, repeating after 150 ms (`main.cpp`); Down is a soft drop read as a level through `keyDown`, and it stops working on a new piece until released, so a held key never carries into the next one. Up slams the piece down. A press of Confirm turns it clockwise. Holding Confirm 700 ms pauses, a bar filling in the side panel from 200 ms; the press has already turned the piece by then, on purpose, so turning never waits. The D-pad's opposite keys may not press together, which rules out chords. `loop()` handles each key on its own rather than in an else-if chain, so a slide and a turn in the same frame both land.

## Architecture

**No blocking `delay()`.** Elapsed time uses unsigned subtraction (`now - last`), deadlines a signed difference. Arduino defines `FALLING`, `RISING` and `CHANGE` as macros, so no enum here can use those names.

- **`src/stack/`** is plain C++ with no Arduino or TFT include. `Game` is the well (two hidden rows above the visible 21), the piece and the rules, and has no clock: `advance(ms, soft)` runs gravity and the lock delay, and the screen paces the clear and the settling, its animations, through `clearRows()` and `settleStep()`. Pieces come from a shuffled bag of seven, and turn in a box with our own simple kicks (`KICKS`). The lock restarts on a slide or turn at most 12 times per row reached. A clear scores 1, 3, 5 or 9 times the level times the chain link; five digits at most, for the side panel. In Cascade a clear leaves the rest where it is, then clumps of touching cells that rest on nothing fall a row per `settleStep()` as rigid shapes, and full rows found once they stop start the next link of the chain. `MODE_NAMES` holds the modes. `Records` (mode, best per mode, runs per mode) goes through `SlotStore`; Sprint's best is tenths of a second, lower being better; adding a field means bumping `RECORDS_VERSION`.
- **`src/ui/BlockArt`** draws one cell, `s` pixels a side, in the active theme's `BlockStyle`, painting its whole square so nothing needs erasing. `src/ui/WellView` remembers what each cell shows, and `sync()` repaints only what differs, so a moving piece costs a dozen cells and never flickers. `wipe()` and `fillRow()` paint over that memory.
- **Screens:** a loop of Title (a logo of blocks that drop in, Play, Mode, Theme, each mode's best, the controls) ↔ Settings (sound, dimming, game menu, reset). `PlayScreen` is a modal opened from the title and takes every arrow. It draws the well straight to the panel from `update()`; the side panel's text is elements, the previews and the pause bar are drawn. A run left through Pause still counts for the best score, and Sprint's only when finished. The end box ignores Confirm for 500 ms, so a player mashing as the stack tops out cannot pick from it unseen.
- **Sound** (`SoundManager.h`): only short blips of our own, no tune. A tick per slide, two notes per turn, a thud per landing, a low run per slam, a climb per clear (more rows, longer), a rising pair per chain link. Quiet keeps clears, levels and the end of a run.

### Colours
**Every colour comes from `src/ui/Theme.h`** as a `Color` slot: the ones `lib/dotui` draws with, then `WELL`, `GRID`, seven pieces (`Theme::piece`), `FLASH` and `DEAD`. Each `ThemeDef` in `Themes.h` also names its block style. Bevels, shadows and the ghost are mixed from those in `BlockArt.cpp`.

### Persistence
NVS namespace `empilha`. Records are written when the mode changes, when a run starts and when one sets a best; settings live under their own key.
