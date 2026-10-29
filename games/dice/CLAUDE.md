# CLAUDE.md

This file provides guidance to Claude Code (claude.ai/code) when working with code in this repository.

## Project

Dice Bag, a table-top helper for an ESP32 with a 240x320 ILI9341 SPI display, five push buttons and a passive buzzer (the same board as `games/idle`). PlatformIO + Arduino framework, C++. Its engine (core, UI, settings, saves) came from `games/deepwell`. Three tools: RPG dice (d2 to d100, up to 10 of them, a bonus, keep high/low and drop lowest, criticals), a number draw with or without repeats (a raffle or a bingo), and a wheel of 2 to 12 slices; plus the history of every result.

## Code style

**Comment sparingly.** Write a comment only when the code cannot say it: a non-obvious constraint, a hardware quirk, or why something that looks wrong is deliberate. Do not narrate what the next line does, restate a name, label sections, or explain a change in a comment (that goes in the commit).

## Commands

`pio` is not on `PATH`; it lives at `~/.platformio/penv/bin/pio`.

```bash
pio run                              # Build
pio run -t upload -t monitor         # Flash and open the serial monitor (115200)
pio run -e dev -t upload -t monitor  # Dev build: serial commands screen <i>, shot
~/.platformio/penv/bin/python tools/screenshot.py out.png [--screen 2]  # Dev build on the board: the display as a PNG
tools/test.sh                        # Logic tests: fairness of every tool, notation, keeps, the hat, the wheel, the save
tools/shots.sh [out_dir]             # The real screens drawn on the PC, one PNG per scripted scene
```

`tools/test.sh` compiles `src/dice/` and `lib/dotcore`'s `SlotStore.cpp` against `tools/stub/`. It checks with a chi-square that every die, the first number out of the hat and every slice of the wheel come out evenly, and the keeps' known means (advantage 13.82, 4d6 drop lowest 12.24). **Rerun it after touching `src/dice/`.**

`tools/shots.sh` builds the screens against `tools/stub/TFT_eSPI.h`, a framebuffer that draws with TFT_eSPI's own GLCD and font-7 data from `.pio/libdeps` (it builds the firmware once if that is missing). `tools/make_mockups.py` takes the site's `screen.png` from its `02_roll_critical` scene.

## Hardware

- **All TFT_eSPI setup lives in the repo root's `common.ini` (`[board]`)**. Fonts 1 (GLCD) and 7 (48px seven-segment digits, only `0-9 : - .`, for the totals) are loaded.
- **Pins are `const int` at the top of `src/main.cpp`**: right 23, left 26, confirm 27, up 19, down 22, buzzer 32.
- **Backlight** (`TFT_BL`, 21) is PWM on LEDC channel 4 (`Backlight` in `lib/dotcore`), since `tone()` owns channel 0. It dims after the Settings delay, and the press that wakes it is swallowed.
- Hold Up + Down while powering on to erase the save; Settings also has a reset.
- **The game menu:** when the site installed the launcher, Settings has a Game menu row (`returnToLauncher()` in `lib/dotcore`'s `Launcher.h`), and holding Confirm while powering on opens it.

## Architecture

**No blocking `delay()`.** Elapsed time uses unsigned subtraction (`now - last`), deadlines a signed difference.

### Layers
- **`src/dice/`** is plain C++ with no Arduino or TFT include. Every random number goes through `uniform()` (`Random.h`), which rejects the raw values that would bias a `% n`, from a `RandomSource`: `esp_random()` on the board, a seeded generator in the tests. `Roll` scores a pool (`scoreDice`: which dice count, the outcome); a critical or fumble needs exactly one kept d20. `Raffle` keeps a bitset of 0-9999 and draws the k-th number still in the hat. `Spinner` draws the winning slice first and shapes the spin to land on it: exponential friction makes the launch speed cover a known distance, and the angle is a closed form of the elapsed time, so the frame rate never changes where it stops. `History` is a RAM ring of 30 results.
- **`src/core/`**: `ScreenManager` and `SoundManager` (queued `tone()` notes; a click is shorter than the gap between two, or the queue falls behind).
- **`src/ui/` and `src/screens/`** follow the engine's rules: every element tracks `dirty` and erases only what changed; mutators are no-ops on an unchanged value; elements are members registered once with `addElement` (`addElement(&e, Theme::CARD)` on a card). Overlapping elements cannot erase each other: `RollScreen` clears its whole dice table (`table`) and redraws every die whenever they move.

### Screens and input
A loop, Left and Right stepping through it: Roll → Draw → Wheel → History → Settings. Roll, Draw and Wheel are `ToolScreen`s: settings rows (`FieldRow`) above the row that acts (`ActionRow`, with the Confirm cap). Every visit starts on the action row, so Confirm rolls at once. Up/Down move between rows; Confirm on a setting edits it until the next Confirm, during which Up/Down change it by one and Left/Right by `bigStep()` (5 for the bonus, tens below 100 and hundreds above for the draw's bounds), all repeating while held (`main.cpp`). A finished edit that changed something saves the prefs. While dice tumble, numbers shuffle or the wheel turns, presses wait; leaving the screen settles the result at once. The result was decided, and logged to the history, on the press.

`WheelView` paints the wheel into a 176x176 `TFT_eSprite` (62 KB) and pushes it whole; the sprite is freed when the screen is left.

### Colours
**Every UI colour comes from `src/ui/Theme.h`** as a `Color` slot; store a `Color`, never a `uint16_t`. The palettes in `Themes.h` came from the idle game, so some slots are unused. A die is drawn in `TEXT` with `CARD` ink, so it is a white die in dark themes and an ink one in light ones. The wheel's slices take `GOLD, INFO, BAD, GOOD, EPIC, MYTHIC, DEFENSE` in turn, and a label picks black or white by its slice's brightness.

### Persistence
NVS namespace `dicebag`. `Prefs` (`dice/Prefs.h`: the dice, the draw's bounds and rule, the slices) goes through `SlotStore`, written when an edit ends; settings live under their own key. Adding a field means bumping `PREFS_VERSION`. The history and the raffle's hat are RAM only, so a power cut refills the hat.
