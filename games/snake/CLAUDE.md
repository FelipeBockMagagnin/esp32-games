# CLAUDE.md

This file provides guidance to Claude Code (claude.ai/code) when working with code in this repository.

## Project

Snake, the grid-snake genre on a dot-matrix LCD look, for an ESP32 with a 240x320 ILI9341 SPI display, five push buttons and a passive buzzer (the same board as `games/idle`). PlatformIO + Arduino framework, C++. Its engine (core, settings, saves) came from `games/dice`. Four levels (Easy wraps through the walls; Normal, Hard and Insane kill on them, the last two speeding up per food), a best score per level, ten themes.

## Code style

**Comment sparingly.** Write a comment only when the code cannot say it: a non-obvious constraint, a hardware quirk, or why something that looks wrong is deliberate. Do not narrate what the next line does, restate a name, label sections, or explain a change in a comment (that goes in the commit).

## Commands

`pio` is not on `PATH`; it lives at `~/.platformio/penv/bin/pio`.

```bash
pio run                              # Build
pio run -t upload -t monitor         # Flash and open the serial monitor (115200)
pio run -e dev -t upload -t monitor  # Dev build: serial commands screen <i>, shot
~/.platformio/penv/bin/python tools/screenshot.py out.png [--screen 1]  # Dev build on the board: the display as a PNG
tools/test.sh                        # Logic tests: turns, walls, wrapping, eating, speed-up, food spread, the save
tools/shots.sh [out_dir]             # The real screens drawn on the PC, one PNG per scripted scene
```

`tools/test.sh` compiles `src/snake/` and `lib/dotcore`'s `SlotStore.cpp` against `tools/stub/`. **Rerun it after touching `src/snake/`.** `tools/shots.sh` builds the screens against `tools/stub/TFT_eSPI.h` (fonts from `.pio/libdeps`, so it builds the firmware once if that is missing) and plays them with an autopilot; `tools/make_mockups.py` takes the site's `screen.png` from its `04_swallowed` scene.

## Hardware

- **All TFT_eSPI setup lives in the repo root's `common.ini` (`[board]`)**; only the GLCD font.
- **Pins are `const int` at the top of `src/main.cpp`**: right 23, left 26, confirm 27, up 19, down 22, buzzer 32.
- **Backlight** (`TFT_BL`, 21) is PWM on LEDC channel 4 (`Backlight` in `lib/dotcore`), since `tone()` owns channel 0.
- Hold Up + Down while powering on to erase the save; Settings also has a reset.
- **The game menu:** when the site installed the launcher, Settings has a Game menu row (`returnToLauncher()`), and holding Confirm while powering on opens it.

## Architecture

**No blocking `delay()`.** Elapsed time uses unsigned subtraction (`now - last`), deadlines a signed difference.

- **`src/snake/`** is plain C++ with no Arduino or TFT include. `Snake` is a 22x28 grid: a ring buffer of segments (segment 0 the head), each keeping the direction the snake moved to enter its cell, which is all the renderer needs to join neighbours. Turns are queued two deep, so a quick U-turn spans two steps; a reversal is dropped. The tail leaves its cell in the same step the head moves, so the head may follow right behind it. Food lands on the k-th free cell, uniformly. `LEVELS` holds each level's speed, speed-up, points and whether walls wrap. `Records` (level + best per level) goes through `SlotStore`; adding a field means bumping `RECORDS_VERSION`.
- **`src/ui/CellArt`** draws one 10x10 cell, its unlit LCD dot included, so a cell never needs erasing: head (eyes that blink, a mouth open when the food is straight ahead), body (a scale dot on every other cell, by grid parity, so it never has to move), the fat cell where food was swallowed (stays until the tail passes it), the tapering tail, the pulsing food. Shapes are written facing right and turned.
- **Screens:** a loop of Title (Play, Level, Theme, the best scores) ↔ Settings (sound, dimming, game menu, reset). No tutorial: rows carry a title and a value only, no subtitles or hint lines, and `LISTROW_CENTER_LONE` centres the lone titles. Changing the theme on the title repaints it whole, logo and crawler included. `PlayScreen` is a modal opened from the title, and takes every arrow, so Left/Right steer instead of switching screens. It draws the field straight to the panel from `update()` (it holds the `TFT_eSPI`), redrawing only the cells a step changed; only the score line is made of elements. Pause and game over draw a box over the field; closing it repaints the field whole. `main.cpp` passes every press of a frame through, so two arrows pressed together both reach the queue. The title's crawler is a fixed-length snake on the ring of cells round the logo, eating food placed on the ring.

### Colours
**Every colour comes from `src/ui/Theme.h`** as a `Color` slot: the ones `lib/dotui` draws with, then `FIELD`, `GHOST` (unlit dots), `INK` and `FOOD`. The light themes give the whole screen the LCD's colour; the dark ones are backlit, the field a shade off the background.

### Persistence
NVS namespace `snake`. Records are written when the level changes and when a game sets a best; settings live under their own key.
