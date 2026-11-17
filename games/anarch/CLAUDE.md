# CLAUDE.md

This file provides guidance to Claude Code (claude.ai/code) when working with code in this repository.

## Project

Anarch, the public-domain (CC0, code and art) Doom-like by drummyfish, ported to an ESP32 with a 240x320 ILI9341 SPI display, five push buttons and a passive buzzer (the same board as `games/idle`). PlatformIO + Arduino framework, C++. The whole game is `src/anarch/`, **vendored unchanged** from `https://gitlab.com/drummyfish/anarch` (commit `6f90562`, 2023-09-08; the Codeberg copy is gone); this project is only its front end, `src/main.cpp`, modelled on upstream's `main_espboy.ino`.

## Code style

**Comment sparingly.** Write a comment only when the code cannot say it: a non-obvious constraint, a hardware quirk, or why something that looks wrong is deliberate. Do not narrate what the next line does, restate a name, label sections, or explain a change in a comment (that goes in the commit). Never edit `src/anarch/`: a change goes through its `#define`s in `src/AnarchConfig.h` or in the front end.

## Commands

`pio` is not on `PATH`; it lives at `~/.platformio/penv/bin/pio`.

```bash
pio run                              # Build
pio run -t upload -t monitor         # Flash and open the serial monitor (115200)
pio run -e dev -t upload -t monitor  # Dev build: fps every 5 s
tools/shot.sh out.png [key:frames]...  # The game on the PC: taps through the menu into level 1, plays the moves, saves the frame
```

`tools/shot.sh` fails if level 1 never starts or the game exits, so it doubles as a smoke test after touching `AnarchConfig.h`. `screen.png` is one of its frames, drawn by `tools/make_mockups.py`.

## Hardware

- **All TFT_eSPI setup lives in the repo root's `common.ini` (`[board]`)**.
- **Landscape**, as in `games/decaf`: held a quarter turn clockwise from the other games, D-pad under the left thumb. The pins at the top of `src/main.cpp` are named for the way each button points held like that. If the picture comes out upside down, the fix is `ROTATION`, not the pins.
- **Backlight** (`TFT_BL`, 21) is PWM on LEDC channel 4 (`Backlight` in `lib/dotcore`), since `tone()` owns channel 0. It never dims: a shooter has no idle moments to wait out.
- Hold Up + Down while powering on to erase the save; hold Confirm to open the game menu (`returnToLauncher()`).

## Architecture

**No blocking `delay()` outside setup**; `SFG_sleepMs` does nothing.

- **`src/AnarchConfig.h`** sets the game's `SFG_*` options and includes `game.h`, for the firmware and the PC tool alike: 160x120 at 30 fps, full raycasting. `SFG_CAN_EXIT` is the runtime `launcherShown`, which `game.h` only reads in an expression, so the menu's Exit row shows only when the launcher is installed; choosing it makes `SFG_mainLoopBody()` return 0, and the front end goes to the launcher.
- **Picture:** `SFG_setPixel` writes palette indices into a 160x120 buffer. When `SFG_game.frame` moved during `SFG_mainLoopBody()`, `pushFrame()` doubles it to 320x240 in bands of 8 source rows, pushed by DMA from two buffers in turn, with `paletteRGB565` byte-swapped into RAM at boot.
- **Controls** (`readKeys()`): Up/Down walk, Left/Right turn, OK is A (fire). OK + Up is the game's C + Up, a jump, which also holds the fire; OK + Down stays A, since C + Down would open the menu. Left + Right together are Next weapon and Up + Down together the menu; either chord mutes its two buttons until both are let go. No strafe.
- **Sound:** each of the six effects of `sounds.h` is a pitch sweep (`SWEEPS`) walked in `updateSound()`, calling `tone()` only when the note changes, since the core queues `tone()` calls. A quieter effect never cuts a louder one. No music. Sound on/off is the game's own menu row.

### Persistence
NVS namespace `anarch`: the game's 12-byte save (levels reached, totals, settings) as one blob under `save`. The game writes it at the end of each level and when its settings change.
