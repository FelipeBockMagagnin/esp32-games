# CLAUDE.md

This file provides guidance to Claude Code (claude.ai/code) when working with code in this repository.

## Project

Arduboy, free and open-source Arduboy games recompiled for an ESP32 with a 240x320 ILI9341 SPI display, five push buttons and a passive buzzer (the same board as `games/idle`). PlatformIO + Arduino framework, C++. Not an emulator: each game's own source (`sketches/<slug>/`) is built against a port of the Arduboy2 library, and every game lives in one firmware, each in a namespace of its own. The engine around them (screens, settings, saves) came from `games/gameboy`.

## Code style

**Comment sparingly.** Write a comment only when the code cannot say it: a non-obvious constraint, a hardware quirk, or why something that looks wrong is deliberate. Do not narrate what the next line does, restate a name, label sections, or explain a change in a comment (that goes in the commit). A game's own code in `sketches/` keeps its author's style; change it only where it cannot run on the board, and list the change in `sketches/README.md`.

## Commands

`pio` is not on `PATH`; it lives at `~/.platformio/penv/bin/pio`.

```bash
pio run                              # Build
pio run -t upload -t monitor         # Flash and open the serial monitor (115200)
pio run -e dev -t upload -t monitor  # Dev build: serial commands screen <i>, play <i>; fps every 5 s
tools/run.sh <slug> out.png [keys:ms]...   # Play a game on the PC under AddressSanitizer, save the panel
tools/run.sh list|settings out.png [theme] # The screens around the games
```

`tools/run.sh` builds the real port, host and games against `tools/stub/` with ASan and UBSan, on a virtual clock, pressing the buttons its script names (`tools/run.cpp` has the keys). **Run every game through it after touching `lib/` or adding a game**: it is how a pointer read as 16 bits, an unaligned load or a read past an array shows up without a board. `screen.png` is a Dark & Under scene drawn this way by `tools/make_mockups.py`.

## The games

- **`library.json`** is the catalogue, in list order: `slug` (the folder in `sketches/`), `title`, `author`, `genre`, `license`, `source`, `ok` (`"B"` when the game's main action is B), optionally `name`/`by` (the board's shorter title and credit; 16 characters fit a list row), `main` (the .ino with `setup()`, when the guess is wrong) and `skip` (folders under the sketch whose .cpp files must not be built). The site lists the games from it through `publish.py`.
- **Only games whose author published them under a licence that allows redistribution**, art included. `sketches/README.md` records each one's upstream, commit and licence, and every change made to it.
- **`tools/sketches.py`** (a PlatformIO pre-script) does what the Arduino IDE does to a sketch: joins its .ino files and adds a prototype of each function, found in the preprocessor's output. It writes, into the build folder, a source that opens the game's namespace (`ab_` and the slug) around the joined .ino and one around each of the game's .cpp files, and the table of games `src/play/Sketches.cpp` includes; a build middleware compiles them beside that file, with `src/`'s flags. They stay out of `src/` because `publish.py` refuses untracked files there.
- **`src/sketches/Prelude.h`** includes every header a game may include, before its namespace opens, so the game's own includes of them do nothing. A game that needs another library: vendor it in `lib/`, name it in `lib_deps`, include it in the prelude and in `sketches.py`'s preprocessing paths.
- **Adding a game:** copy its sketch folder into `sketches/<slug>/` with its licence, add it to `library.json`, build, then play it through `tools/run.sh`. What usually needs fixing:
  - a bundled copy of Arduboy2 or ArduboyTones (timers, `PORTx`): replace it with a header that includes the port's;
  - a function that falls off its end (`-Werror=return-type` stops the build);
  - `::name` meaning a global: it now lives in the game's namespace;
  - AVR registers (`ADC`, `TCCRx`), usually for a random seed: `arduboy.generateRandomSeed()`;
  - `int` is 4 bytes here and 2 there: an EEPROM layout of fixed offsets that `put()`s an `int` overlaps the next field;
  - a 16-bit read from an odd address (casting a byte array): the Xtensa faults where the AVR did not.
  ArduboyPlaytune and ATMlib are not ported, so games that use them are left out for now.

## The port

- **`lib/Arduboy2`** is Arduboy2 6.0.0 (BSD-3-Clause): `Arduboy2Core` rewritten for the board, the AVR assembly in `drawPixel`, `fillScreen` and `Sprites`' plus-mask mode turned into C (keeping the 16-bit offset wrap of a sprite above the screen), `BitStreamReader` moved into the header for games written for 4.x, and `Arduboy2Audio`/`Arduboy2Beep` on the host's two sound channels. **`lib/ArduboyTones`** (BSD-3-Clause) times its notes from the host's tick instead of timer 3. **`lib/FixedPoints`** (Apache-2.0) is unchanged but for allowing its random functions on Arduino. **`lib/AvrShim`** has `avr/*.h` and `EEPROM.h`.
- **`ArduboyHost.h`** is what the library asks of the firmware (`src/play/Host.cpp`). A game owns the loop, so each call it makes into the library is the firmware's turn: `buttonsState()` reads the buttons, `nextFrame()`'s idle waits a millisecond and checks the menu chord, `display()` pushes the frame.
- **`pgm_read_word()`** returns the whole pointer when the table it reads holds pointers, since games keep tables of 16-bit AVR pointers and read them with it, and otherwise reads its two bytes one at a time (`lib/AvrShim/src/avr/pgmspace.h`). `min`, `max` and `abs` take mixed types, as the AVR's macros did (`Prelude.h`'s `abcompat`).
- **EEPROM:** each game has 1 KB of its own, 0xFF but for the system bytes (boot logo off, sound on), loaded from NVS before `setup()` and stored, PackBits-packed, once the game has stopped writing for 1.5 s and whenever the menu opens. Its key is `g` + the FNV-1a of the slug.

## Hardware

- **All TFT_eSPI setup lives in the repo root's `common.ini` (`[board]`)**; `DISPLAY_LANDSCAPE` lays `lib/dotui` out at 320x240.
- **Landscape**, as in `games/anarch`: held a quarter turn clockwise from the other games, D-pad under the left thumb. The pins at the top of `src/main.cpp` are named for the way each button points held like that.
- **Backlight** (`TFT_BL`, 21) is PWM on LEDC channel 4 (`Backlight` in `lib/dotcore`), since `tone()` owns channel 0. It never dims during a game.
- Hold Up + Down while powering on to erase every save; Settings also has a reset. Hold Confirm to open the game menu (`returnToLauncher()`); Settings and the pause menu have a Game menu row when the launcher is installed.

## Architecture

- **`src/play/Host`**: the 128x64 frame drawn at 2x (256x128) at the top, only the 8-row pages that changed, two 16-row bands pushed by DMA in turn; the strip below names the game and the controls. **Controls:** OK is A (B when the entry says so, or after Swap), Left + Right together the other; Up + Down together is never a direction, and held for a second opens the pause menu, which runs inside the library call the game was in. The two speaker channels share the buzzer, the first one sounding over the second.
- **Quitting restarts the board**, since a game's globals cannot be put back as they were: an `RTC_NOINIT_ATTR` word tells the next boot to show the list on that game, or to start it again (Restart game). Starting a game from the list needs no restart; its globals have not been touched.
- **Screens:** the game list (title over author, SAVE on a game with a save) ↔ Settings (sound: on, games only, off; the palette; dimming; game menu; reset).

### Colours
**Every colour comes from `src/ui/Theme.h`** as a `Color` slot: the ones `lib/dotui` draws with, then `PIXEL_OFF` and `PIXEL_ON`, the game screen's two colours. A theme is the panel the games are drawn on and the menus alike; the first is white on black, as the Arduboy's OLED.

### Persistence
NVS namespace `arduboy`: each game's EEPROM under its own key, the settings under theirs.
