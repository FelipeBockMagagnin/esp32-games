# CLAUDE.md

This file provides guidance to Claude Code (claude.ai/code) when working with code in this repository.

## Project

Every game for the dotcompany board (an ESP32 with a 240x320 ILI9341 SPI display, five push buttons and a passive buzzer), in one repo: `games/<id>/` holds one PlatformIO + Arduino project per game, `launcher/` the menu that boots them, `hardware/` the PCB and case, and `docs/` the site that installs them from the browser. `README.md` covers publishing and adding a game. **Each game has its own `CLAUDE.md`; read it before working in that folder.**

## Code style

**Comment sparingly.** Write a comment only when the code cannot say it: a non-obvious constraint, a hardware quirk, a balance number's source, or why something that looks wrong is deliberate.

## Commands

`pio` is not on `PATH`; it lives at `~/.platformio/penv/bin/pio`. Every project builds from its own folder:

```bash
pio run -d games/idle                     # Build one game
pio run -d games/idle -t upload -t monitor
tools/publish.py                          # Rebuild every changed firmware into docs/
tools/publish.py --list                   # Only rewrite docs/games.js and the screens
python3 tools/make_mockups.py             # Redraw games/<id>/screen.png
```

## Shared between games

- **`common.ini`** is the board: platform, TFT_eSPI and its pins, read by every `platformio.ini` through `extra_configs`. A game's env says `extends = board` and appends its own flags to `${board.build_flags}`. The launcher, one level shallower, names its own `lib_extra_dirs`. A change here rebuilds every firmware.
- **`lib/`** holds the engine the games share, as PlatformIO libraries found through `lib_extra_dirs`. Its headers are included by bare name (`#include "Box.h"`).
  - `dotcore`: `ButtonManager`, `Backlight`, `SlotStore`, `Settings`, `pitches.h`, `Launcher.h`.
  - `dotui`: `UI.h` and the elements built on it (`Box`, `Text`, `ListRow`, `ListView`, `Header`, `ProgressBar`, `Image`, `ScreenWipe`...). It includes the game's own `Theme.h`, found through `-I src/ui` in `common.ini`, so every game's Theme must name the slots `dotui` draws with.
  - A per-game variation is a build flag, not a fork: `DISPLAY_LANDSCAPE` (Decaf lays out 320x240), `LISTROW_CENTER_LONE` (JackHammer). **A change in `lib/` rebuilds every firmware: build all of them and run their tests.**
  - A PC build in a game's `tools/` that compiles code from `lib/` needs `-I ../../lib/<name>/src`.
- **The rest of the engine is copied, not shared,** because each game diverged on purpose: `ScreenManager`/`Screen` (newer games repeat any held key), `ConfirmScreen` and `SettingsScreen` (their own layouts), `Theme.h`/`Themes.h` (each game's slots and palettes), `SoundManager`, `SplashArt` and `Theme.cpp` (the game's art and palettes). When code turns out identical in three or more games, move it to `lib/`.
- **NVS**: every firmware keeps its own namespace (`idlegame`, `petgame`, `decaf`, `roguelike`, `pomodoro`, `tilde`, `jackhammer`, `launcher`), its reset clears only that namespace, and all of them share the default partition table's 20 KB `nvs`. A new game must follow suit, or installing it would erase the others' saves.
- **The launcher** (`launcher/CLAUDE.md`) sits in the `factory` partition when the site installs several games. `docs/bundle.js` writes the flash layout and catalog that `launcher/src/Catalog.h` reads, so change both together.

## The site

`tools/publish.py` discovers games from `games/*/game.json`. The folder name is the id, which the site, the firmware folder and the launcher's partition label all use. It validates each `category` against `games/categories.json` and generates `docs/games.js` (`window.CATEGORIES`, `window.GAMES`, `window.LAUNCHER`), from which `docs/app.js` builds the shelves. Never edit `docs/games.js`, `docs/img/<id>.png` or `docs/firmware/` by hand. A game's version is the last commit touching its firmware sources, and the script rebuilds only games whose version differs from the published manifest.
