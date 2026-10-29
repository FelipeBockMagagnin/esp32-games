# CLAUDE.md

This file provides guidance to Claude Code (claude.ai/code) when working with code in this repository.

## Project

The game menu for the dotcompany board (ESP32, 240x320 ILI9341, five buttons, a buzzer: the same carrier board as `../esp32-idle-game`). When the site (`../esp32-games-web`) installs several games at once, this app sits in the `factory` partition and every game in an OTA slot of its own; the menu shows them one at a time and boots the chosen one. PlatformIO + Arduino framework, C++.

## Code style

**Comment sparingly.** Write a comment only when the code cannot say it: a non-obvious constraint, a hardware quirk, or why something that looks wrong is deliberate.

## Commands

`pio` is not on `PATH`; it lives at `~/.platformio/penv/bin/pio`.

```bash
pio run                              # Build
pio run -e dev -t upload -t monitor  # Dev build: serial `shot`, `play <n>`
~/.platformio/penv/bin/python tools/screenshot.py out.png  # Dev build on the board: the display as a PNG
```

A plain `pio run -t upload` puts the menu in `app0` of the default table, with no catalog: it then lists the other app partition by its label, enough to check the drawing.

## How switching works

- **Flash layout** (written by the site's `docs/bundle.js`, never by this repo): the default table's `nvs` (0x9000) and `otadata` (0xE000), so the games' saves survive moving between one game and several; a `catalog` data partition (type 0x40) at 0x10000; this app as `factory` on the next 64 KB boundary; then one `ota_N` per game, labelled with the game's id from `games.json`, each its firmware rounded up to 64 KB. Blank otadata boots the factory app, so a fresh install opens the menu.
- **Into a game:** `esp_ota_set_boot_partition(game)` + `esp_restart()`. From then on the board powers up straight into that game.
- **Back to the menu:** each game carries a small `Launcher.h` whose `returnToLauncher()` sets the boot partition back to this `factory` app and restarts. Games show their "Menu" item only when `launcherInstalled()` finds it, so a single-game install is unchanged.
- **Every game keeps its own NVS namespace** (`idlegame`, `petgame`, `decaf`, `roguelike`, `pomodoro`, `tilde`, `jackhammer`; this app uses `launcher`) and its reset clears only that namespace. A new game must do the same, and must not erase the whole NVS. All their saves share the 20 KB `nvs` partition.
- **Size budget:** this app must stay under the slot the site gives it (its size rounded up to 64 KB, today 320 KB): every 64 KB it grows is room the games lose on a 4 MB board.

## The catalog

`Catalog.h` defines the format: a `GCAT` header, then 80-byte entries (partition label, name, ASCII tagline, RGB565 accent, a 64x64 RGB565 icon offset, a crop of the game's screen drawn at exactly 2x so it stays sharp). Its writer is `docs/bundle.js` in esp32-games-web; **change both together** and bump `CATALOG_VERSION`. An entry whose app partition is missing or holds no valid image is skipped.

## Hardware

- TFT_eSPI setup lives in `platformio.ini` `build_flags`; only the GLCD font (font 1), which has no accents, so the site strips them from the taglines.
- Pins are `const int` at the top of `src/main.cpp`. Backlight is PWM on LEDC channel 4 (`core/Backlight`), since `tone()` owns channel 0.
