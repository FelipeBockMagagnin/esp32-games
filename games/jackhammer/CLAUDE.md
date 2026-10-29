# CLAUDE.md

This file provides guidance to Claude Code (claude.ai/code) when working with code in this repository.

## Project

*The Surviving JackHammer*, a port of the user's Unity mobile game (`~/Downloads/TheSurvivingJackHammer-master`) to an ESP32 with a 240x320 ILI9341 SPI display, five push buttons and a passive buzzer, the same board as `../esp32-idle-game`. The engine (core, UI, settings, saves) came from `../esp32-roguelike`; the DMA band renderer and the 60 Hz step clock from `../esp32-decaf`. PlatformIO + Arduino framework, C++.

A jackhammer stands in the middle of a square arena and turns a quarter at a time. Enemies walk in from the four sides; one that meets its head dies (a point and coins), one that reaches its body ends the round. Coins buy the Unity game's styles and songs.

## Code style

**Comment sparingly.** Write a comment only when the code cannot say it: a non-obvious constraint, a hardware quirk, a number's source in the Unity project, or why something that looks wrong is deliberate. Do not narrate what the next line does, restate a name, label sections, or explain a change in a comment (that goes in the commit).

## Commands

`pio` is not on `PATH`; it lives at `~/.platformio/penv/bin/pio`.

```bash
pio run                              # Build
pio run -t upload -t monitor         # Flash and open the serial monitor (115200)
pio run -e dev -t upload -t monitor  # Dev build: serial commands coins <n>, shot
~/.platformio/penv/bin/python tools/screenshot.py out.png  # Dev build on the board: the display as a PNG
tools/test.sh                        # Logic tests and the bots' scores
tools/frame.sh [seconds] [out.ppm]   # The arena drawn on the PC: every style mid-round, the four facings
python3 tools/convert_sprites.py     # src/assets/png/sprites/ -> src/assets/Sprites.{h,cpp}
```

`tools/test.sh` compiles `src/jack/` and `src/core/SlotStore.cpp` against `tools/stub/` and checks that every side takes the same time from spawn to the head (and that it matches the Unity game's average), the Unity difficulty curve, the scoring, the shop and the save; then bots with a reaction time and a press rate play 300 rounds each and print their scores. **Rerun it after touching `src/jack/Config.h`.**

## Hardware

- **All TFT_eSPI setup lives in `platformio.ini` `build_flags`**, not a `User_Setup.h`. Only the GLCD font (font 1) is loaded.
- **Pins are `const int` at the top of `src/main.cpp`**: right 23, left 26, confirm 27, up 19, down 22, buzzer 32. Buttons debounce at 15 ms, not the engine's 50, since a turn must land at once.
- **The buzzer is not driven by `tone()`** (`core/Buzzer`): the core queues `tone()` notes and plays each to its end, so an effect would wait behind a song note. A FreeRTOS task on core 0 owns LEDC channel 0 and plays the song, an effect cutting in over it while the song keeps time underneath.
- **Backlight** (`TFT_BL`, 21) is PWM on LEDC channel 4. It dims after the Settings delay, and the press that wakes it is swallowed.
- Hold Up + Down while powering on to erase the save; Settings also has a reset.

## Architecture

**No blocking `delay()` in game code.** Elapsed time uses unsigned subtraction (`now - last`), deadlines a signed difference.

### Layers
- **`src/jack/`** is the game, plain C++ with no Arduino or TFT include. `Round` is one round, stepped at 60 Hz; `turn()` and `step()` raise `RoundEvent` bits and leave `Mark`s (where a kill or the death happened) for the screen's sounds and particles. It rolls dice only through its own `Rng` (xorshift), never Arduino's `random()`. `Profile` is what outlives a round (coins, best, the shop); `Save` writes it.
- **`src/render/`** is plain C++ too: `ArenaRenderer` composites the 240x240 arena one 16 px band at a time (background or backdrop, enemies, the hammer turned around its pivot, particles), `Effects` holds the particles, flying coins and shake, `StyleArt` maps each `STYLES` row to its art.
- **`src/core/`** is game-agnostic: buttons, `ScreenManager`, `Buzzer`/`SoundManager`, backlight, settings and `SlotStore`.
- **`src/ui/` and `src/screens/`** follow the engine's rules: every UI element tracks `dirty` and erases only what changed; mutators are no-ops on an unchanged value; elements are members registered once with `addElement`. Store colours as `Theme` `Color` slots, never `uint16_t`; the art keeps its own RGB565.

### Screens
`PlayScreen` is the only screen in the rotation and consumes Left and Right. On the title Confirm starts a round, Up opens the shop and Down the settings; in a round Left/Right turn and Confirm turns clockwise (the Unity game's Space). The arena (y 40-279) is pushed by `pushFrame()` with `pushImageDMA` in alternating band buffers, under a text HUD and above a footer. A death freezes the field for `DYING_MS`, then `died()` in `main.cpp` banks the coins and opens `GameOverScreen` (continue, play again, title). `ShopScreen`, `SettingsScreen` and the reset `ConfirmScreen` are modals.

### Balance (`src/jack/Config.h`)
The Unity numbers are kept in Unity units (`PX_PER_UNIT` 20, so the 11x20 pixel art is drawn 2x). The Unity spawns sat at different distances per side (7 to 9.5 units), so the time to react depended on the side; **the arena is square so it does not**, and `ENEMY_SPEED` is scaled so the time from spawn to the head equals the Unity average (1.23 s). The spawn interval follows `DifficultyManager` (1.3 s, -0.06 a second down to 0.8, then -0.006). Its speed ramp never reached the enemies in the Unity game (`Enemy1` read `initialEnemy1Speed`), so the speed stays constant here too. A kill pays `points / 10 + 1` coins, counted before the point as the Unity frame order did. Continuing costs what the round earned (at least `CONTINUE_MIN_PRICE`), once a round.

### Persistence
`SlotStore` writes one record to NVS, alternating two slots with a sequence number and a checksum. `SaveBlob` (`jack/Save.h`) holds the `Profile`, written at each death, purchase and continue; settings live under their own key. `Profile` stays POD; adding a field means bumping `SAVE_VERSION`, and a new shop row changes the layout fingerprint. A mismatched save is discarded.

### Content
`STYLES` and `SONGS` in `Config.h` carry the Unity market's prices. A style needs a row in `render/StyleArt.h` (art, background colours from the Unity `ChangeBackgroundColor` arrays, shop icon) and a song a track in `audio/Songs.h`, in the same order (`static_assert`ed). The songs are original loops after the moods of the Unity soundtrack, which a passive buzzer cannot play.

### Assets
`src/assets/png/sprites/` holds the Unity PNGs. `tools/convert_sprites.py` scales each to its size in the Unity scene (sprite pixels at 100 per unit times the prefab's scale), keeps the `.png.meta` pivot, crops to the opaque box shared by its frames, and stores palette indices with one palette per art (`Art` in `assets/Art.h`). Its `ARTS` table holds the frame rects, pivots and prefab scales copied from the Unity project. Never edit `Sprites.{h,cpp}` by hand.

## Performance
A frame is 15 bands of 240x16, about 17 ms of SPI at 55 MHz; the renderer's inner loops are table lookups. Avoid `String` in refresh paths: `char buf[N]` + `snprintf` + `setText(const char *)`.
