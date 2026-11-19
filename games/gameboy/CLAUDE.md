# CLAUDE.md

This file provides guidance to Claude Code (claude.ai/code) when working with code in this repository.

## Project

Game Boy, an emulator with free and open-source homebrew built in, for an ESP32 with a 240x320 ILI9341 SPI display, five push buttons and a passive buzzer (the same board as `games/idle`). PlatformIO + Arduino framework, C++. Its engine (core, settings) came from `games/snake`; the emulator is Peanut-GB (`src/gb/peanut_gb.h`, MIT, vendored unchanged). Two editions build from this source: this folder is the lite one (the games that fit the default table's 1.25 MB app slot, so it installs beside others), and `games/gameboymax` packs every game into a partition table of its own and installs alone.

## Code style

**Comment sparingly.** Write a comment only when the code cannot say it: a non-obvious constraint, a hardware quirk, or why something that looks wrong is deliberate. Do not narrate what the next line does, restate a name, label sections, or explain a change in a comment (that goes in the commit).

## Commands

`pio` is not on `PATH`; it lives at `~/.platformio/penv/bin/pio`.

```bash
pio run                              # Build the lite edition
pio run -d ../gameboymax             # Build the max edition
pio run -t upload -t monitor         # Flash and open the serial monitor (115200)
pio run -e dev -t upload -t monitor  # Dev build: serial commands screen <i>, play <i>, shot; fps every 5 s
tools/fetch_roms.py                  # Download the ROMs library.json lists that roms/ lacks; refuses what Peanut-GB cannot run
tools/test.sh                        # Logic tests, and every ROM in library.json booting for a minute
tools/shots.sh [lite|max] [out_dir] [theme] [game[:frames]]...  # The list, settings and games drawn on the PC
```

**Rerun `tools/test.sh` after touching `src/emu/` or adding a game.** `tools/shots.sh` reads the pack from the firmware build, so build the edition first; `screen.png` (here and in `../gameboymax`) is one of its game scenes.

## The games

- **`library.json`** is the catalogue, in list order: `slug` (the Homebrew Hub's, and the ROM's name in `roms/`), `title`, `author`, `genre`, `license`, `source` (the code or page the licence points to), `file` (the ROM's name in the Hub's database), `ok` (`"B"` when the game's main action is B), `lite` (in the lite edition too), and optionally `name`/`by`, the board's shorter title (16 characters fit a list row) and credit. `fetch_roms.py` adds `size` and `save`. The site lists the games from this file through `publish.py`.
- **Only games whose author published them under a licence that allows redistribution** go in, and only when their art is under it too: Tuff and Porklike GB were left out because their code is free but their art is not. Game Boy Color-only ROMs (header 0xC0) cannot run on Peanut-GB.
- **`tools/pack.py`** (a PlatformIO pre-script, the tier from `custom_tier`) lays the ROMs out in one blob in the build folder and links it as `gb_pack_start`; `src/emu/Pack.h` reads it, so change both together. The lite edition's flash is at 98%: a game added to it has to replace one.

## Hardware

- **All TFT_eSPI setup lives in the repo root's `common.ini` (`[board]`)**; only the GLCD font.
- **Pins are `const int` at the top of `src/main.cpp`**: right 23, left 26, confirm 27, up 19, down 22, buzzer 32.
- **Backlight** (`TFT_BL`, 21) is PWM on LEDC channel 4 (`Backlight` in `lib/dotcore`), since `tone()` owns channel 0.
- Hold Up + Down while powering on to erase every save; Settings also has a reset.
- **The game menu:** when the site installed the launcher, Settings and the pause menu have a Game menu row (`returnToLauncher()`), and holding Confirm while powering on opens it.

## Architecture

**No blocking `delay()` outside setup.**

- **`src/emu/`** is plain C++ with no Arduino or TFT include, so the PC tests run it. `GbCore.c` compiles Peanut-GB once, as C at -O2; `GameBoy` wraps it: the ROM read straight from flash, the cartridge RAM on the heap, a CPU error escaping through `longjmp` (Peanut-GB must not return from `gb_error`). `Apu` keeps the sound registers and boils them down to one note a frame, the loudest of the two square channels and the wave channel, for the buzzer. `Scale` draws 160x144 at 1.5x (240x216). `SaveCodec` is PackBits; `Saves` keeps each battery game's RAM packed under its own key, refusing one that packs to more than 2 KB, since every firmware shares the 20 KB NVS.
- **`src/play/Player`** runs a game outside the screens, since it needs the buttons' levels each frame and the whole SPI bus. Peanut-GB draws every other frame (`frame_skip`); 16 lines make a band of 24 rows, pushed by DMA while the next lines are drawn into the other buffer. The strip below names the game and the controls, or holds the pause menu. **Controls:** OK is A (B when the entry says so, or after Swap in the menu), Left + Right together the other one, Up + Down tapped together Start (sent on release); holding Up + Down for a second pauses instead. A chord lasts until both its buttons are up, so it never leaks a direction. The buttons debounce in 15 ms, not the library's 50: read once a frame, a quick tap would vanish. A battery game saves 5 s after it last wrote its RAM, when the menu opens and when it quits.
- **Screens:** the game list (title over author, SAVE on a game with a save) ↔ Settings (sound: on, games only, off; the palette; dimming; game menu; reset).

### Colours
**Every colour comes from `src/ui/Theme.h`** as a `Color` slot: the ones `lib/dotui` draws with, then `SHADE_0` to `SHADE_3`, the four Game Boy shades lightest first. A theme is a palette for the games and the menus alike.

### Persistence
NVS namespace `gameboy`, **shared by both editions on purpose**: the same emulator with the same games under the same keys, so moving from one to the other keeps the saves. A save's key is `g` + the FNV-1a of the slug in hex; settings live under their own key.
