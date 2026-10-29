# CLAUDE.md

This file provides guidance to Claude Code (claude.ai/code) when working with code in this repository.

## Project

Deepwell, a turn-based dungeon crawler for an ESP32 with a 240x320 ILI9341 SPI display, five push buttons and a passive buzzer (the same carrier board as `games/idle`, whose UI framework this started from). PlatformIO + Arduino framework, C++. Floors are generated from a seed; every step is a turn; death ends the run and only the records survive.

## Code style

**Comment sparingly.** Write a comment only when the code cannot say it: a non-obvious constraint, a hardware quirk, a balance number's source, or why something that looks wrong is deliberate. Do not narrate what the next line does, restate a name, label sections, or explain a change in a comment (that goes in the commit).

## Commands

`pio` is not on `PATH`; it lives at `~/.platformio/penv/bin/pio`.

```bash
pio run                              # Build
pio run -t upload -t monitor         # Flash and open the serial monitor (115200)
pio run -e dev -t upload -t monitor  # Dev build: serial commands floor <n>, shot
~/.platformio/penv/bin/python tools/screenshot.py out.png  # Dev build on the board: the display as a PNG
tools/test.sh                        # Logic tests and the autoplay difficulty probe
python3 tools/convert_sprites.py     # Regenerates src/assets/Sprites.{h,cpp} from src/assets/png/tiles/
```

`tools/test.sh` compiles `src/rogue/` and `lib/dotcore`'s `SlotStore.cpp` for the PC against `tools/stub/` (Arduino and an in-memory `Preferences`). It checks the generator over 12000 seeded floors (stairs reachable, nothing spawned in a wall, the same seed giving the same floor), combat and the save slots, then plays 200 runs with a greedy bot and prints the floors it dies on and what killed it. **Rerun it after touching a table in `src/rogue/Config.h`**; the autoplay numbers in its comments come from it.

## Hardware

- **All TFT_eSPI setup lives in the repo root's `common.ini` (`[board]`), shared by every game**, not a `User_Setup.h`. Only the GLCD font (font 1) is loaded; enabling another costs flash.
- **Pins are `const int` at the top of `src/main.cpp`**, the same as the idle game's: right 23, left 26, confirm 27, up 19, down 22, buzzer 32.
- **Backlight** (`TFT_BL`, 21) is PWM on LEDC channel 4 (`Backlight` in `lib/dotcore`), since `tone()` owns channel 0 and its timer. It dims after the Settings delay, and the press that wakes it is swallowed.
- Hold Up + Down while powering on to erase the save; Settings also has a reset.
- **The game menu:** when the site installed the launcher, Settings has a Game menu row (`returnToLauncher()` in `lib/dotcore`'s `Launcher.h`), and holding Confirm while powering on opens it.

## Architecture

**No blocking `delay()` in game code.** Elapsed time uses unsigned subtraction (`now - last`), deadlines a signed difference (`(long)(now - deadline) >= 0`).

### Layers
- **`src/rogue/`** is the game, plain C++ with no Arduino or TFT include, so the tests build it on the PC. `Run` is the whole state; `act(Action)` plays one turn and returns `Event` bits that the screen turns into sounds. **A run is deterministic from its seed and the player's actions**: a floor comes from `Rng(seed ^ floor * K)` (`Rng.h`, xorshift32) and nothing else rolls dice, so never call Arduino's `random()` in `src/rogue/`.
- **`src/core/`** is game-agnostic: `ScreenManager` and sound. Buttons, backlight, settings and `SlotStore` live in the repo's `lib/dotcore`.
- **`src/ui/` and `src/screens/`** came from the idle game. The rules there still hold: every UI element tracks `dirty` and erases only what moved; every mutator is a no-op on an unchanged value; elements are members registered once with `addElement`, never added at runtime; an element on a card is registered with `addElement(&e, Theme::CARD)`.

### Screens and input
`DungeonScreen` is the only screen in the rotation; everything else is a modal (`MenuScreen` on Confirm, `SettingsScreen` from the menu, `ConfirmScreen` for reset, `GameOverScreen` on death). The dungeon consumes all four directions: Left through `onBackPress`, Right through `onRightPress`, both returning true. Holding a button sends `onRepeat(Key)` at a steady pace (`main.cpp`); the dungeon keeps walking only while each step is a plain move, so holding never fights, takes a hit or walks off the stairs.

`TileMap` remembers what each cell last showed (tile and glyph) and repaints only changed cells, compositing the glyph over the tile in a 16x16 buffer. A cleared screen (`resetDrawn`) repaints them all.

### Colours
**Every UI colour comes from `src/ui/Theme.h`** as a `Color` slot resolved against the active palette at draw time; store a `Color`, never a `uint16_t`. The palettes in `Themes.h` came from the idle game, so some slots are unused here. Sprites are art and keep their own colours.

### Persistence
`SlotStore` writes one record to NVS, alternating two slots with a sequence number and a checksum, so a power cut mid-write only loses that write. `SaveBlob` (`rogue/Save.h`) holds the run's `Snapshot` and the `Records`; settings live under their own key. **The run is saved at the start of each floor** (new run, stairs) and on death (as no run, with the records updated): a power cut replays the floor from its start. Snapshots stay POD; adding a field means bumping `SAVE_VERSION`, and a table row is caught by the layout fingerprint. A mismatched save is discarded.

### Content is table-driven
`src/rogue/Config.h`: `ENEMIES` (each floor past its first adds 10% hp and attack), `ITEMS` (`ItemId` must follow its order), and the pacing constants. Damage is `damageThrough`, `attack - defense` **floored at 1**, unlike the idle game: a fight the player cannot win by attrition would stall a turn-based run. A new `Glyph` needs a sprite in `TileMap.cpp`'s `GLYPH_PIXELS` (a `static_assert` checks the count).

### Assets
PNGs in `src/assets/png/tiles/` become 16x16 byte-swapped RGB565 icons, black transparent, through `tools/convert_sprites.py` (never edit `Sprites.{h,cpp}` by hand). Larger PNGs (the 64x64 enemies from the idle game) are cropped and box-filtered down. Each icon costs 512 B of flash.
