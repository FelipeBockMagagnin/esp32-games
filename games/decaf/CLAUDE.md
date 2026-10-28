# CLAUDE.md

This file provides guidance to Claude Code (claude.ai/code) when working with code in this repository.

## Project

*Decaf Inferno*, a side-scrolling platformer for an ESP32 with a 240x320 ILI9341 SPI display, five push buttons and a passive buzzer: the same board as `esp32-idle-game`, whose engine (UI, screens, saves, themes) this project started from, by way of `esp32-pet`. The coffee machine broke and HR won't buy another: a programmer with an empty mug runs each floor of the office left to right, and at the end a coworker (Bea) tells him this floor's machine is broken too, so he goes one floor down. The floors are Dante's nine circles; B1 Limbo is built, with its boss. The game design comes from the Godot prototype in `../jogo-cafe`, whose physics constants are ported as they are (`Stage.cpp`).

## Code style

**Comment sparingly.** Write a comment only when the code cannot say it: a non-obvious constraint, a hardware quirk, a tuning number's source, or why something that looks wrong is deliberate. Do not narrate what the next line does, restate a name, label sections, or explain a change in a comment (that goes in the commit). Most functions and members need none.

## Commands

`pio` is not on `PATH`; it lives at `~/.platformio/penv/bin/pio`.

```bash
pio run                       # Build
pio run -t upload -t monitor  # Flash and open the serial monitor (115200)
tools/sim/test.sh             # Logic tests, including a deathless run through every floor
tools/sim/film.sh map [level] [out.png]            # The whole floor in one strip, with the game's renderer
tools/sim/film.sh frame <level> <steps> [out.png]  # The view after the solver's route plays that long
python3 tools/convert_sprites.py                   # PNGs in src/assets/png/sprites/ -> Sprites.{h,cpp}
```

The tests compile the real `src/game/` against `tools/sim/stub/` (which fakes `Arduino.h` and an in-memory `Preferences`). `tools/sim/Solver.h` is a beam search that plays the real `Stage` with short held inputs, guided by the open-cell distance to the coworker plus the boss's hits; **rerun the tests after touching a map or a physics constant**, since a route that never dies is the proof a floor can be finished. The solver finds shortcuts a player might (on B1 it bounces off the drunk on the desk over the wall-jump shaft); that is fine as long as the intended way also works.

## Hardware

- **All TFT_eSPI setup lives in `platformio.ini` `build_flags`**, not a `User_Setup.h`. Only the GLCD font (font 1) is loaded.
- **Landscape.** The board is held on its side, a quarter turn clockwise from the other games, so the D-pad is under the left thumb. `ROTATION` and the pins at the top of `src/main.cpp` are named for the way each button points held like that (the other games' Up pin is Right here). If the picture comes out upside down, the fix is `ROTATION` 1 ↔ 3, not the pins. The engine's `TFT_WIDTH`/`TFT_HEIGHT` stay the portrait panel's; screens lay out with `DISPLAY_W`/`DISPLAY_H` (`src/ui/UI.h`).
- **Controls in a floor:** Left/Right run, Confirm (red) jumps (letting go early cuts the jump; held through a stomp, the bounce goes twice as high), Down dashes, Up pauses. Buttons debounce at 15 ms instead of the engine's 50, which a jump feels.
- **Global gestures** (hold Left 1.5 s for Settings, Up + Down 3 s for reset) are **off during a floor**, where those buttons play; Settings is in the pause menu and on the title. Up + Down while powering on still erases the save.
- **Backlight** (`TFT_BL`, 21) is PWM on LEDC channel 4, since `tone()` owns channel 0 and its timer. `tone()` queues its notes, so every sound heard in play is a few tens of milliseconds.

## Architecture

### Main loop and screens
`src/main.cpp` owns every object as a global. The rotation is Title → Floor (`LevelScreen`), both consuming Left/Right so the rotation never moves by itself; `startFloor()` calls `LevelScreen::begin()` and switches. Splash, Settings and the reset `ConfirmScreen` are modals. The main loop passes the held buttons to `LevelScreen::setHeld()` every loop; presses (jump, dash, pause) arrive through the screen handlers and are latched until the next step.

### The floor: `Stage` (logic) and `Renderer` (pixels)
Both are plain C++ with no TFT dependency, so the PC tools run them unchanged.

- **`src/game/Stage`** is the whole simulation, stepped at a fixed **60 Hz** (`STEPS_PER_SECOND`) whatever the frame rate: `LevelScreen` keeps a credit in thirds of a millisecond and runs at most 4 steps per frame, dropping the rest, so a stall never bursts. Positions are float pixels; bodies collide with the tile grid one axis at a time (`moveBody`), desks and crumbling platforms only from above. The player is a port of the prototype's `player.gd` state machine (idle, run, jump, fall, wall slide, dash, plus dead and talking), with coyote time and a jump buffer. `takeEvents()` returns the step's `StageEvent` bits, which the screen turns into sounds; `collectDraws()` lists the sprites back to front.
- **The stomp rule** (from the prototype's `AndarilhoMovement.gd`): touching an enemy while falling with the feet no lower than its top + 8 px (`STOMP_MARGIN`) kills it and bounces; any other touch kills the player. A death pops him up, then restarts **everything** from the last checkpoint reached; deaths are counted, there are no lives.
- **The boss** waits until the player is three cells into the arena, then the entry gate shuts (`ENTRY_GATE` is solid only during the fight; `EXIT_GATE` until he falls) and the camera locks on the arena. He paces, leans back, charges to a wall and is dizzy there: only then does a stomp hurt him (3 hits, each charge faster). On his head while awake the player is only thrown off.
- **`src/render/Renderer`** composites the 320x224 play area one 16 px band at a time (one row of tiles): the office backdrop drawn from the level's `Backdrop` colours with parallax, the tiles, then the sprites. `LevelScreen::pushFrame()` alternates two band buffers and sends them with `pushImageDMA`, drawing the next band while the last one goes out; a frame is about 30 ms of SPI. The 16 px HUD above it is ordinary UI elements.
- **Overlays** (intro and results cards, pause menu, the dialogue box) are UI elements drawn over a frozen frame. To take one away, `LevelScreen::lift()` marks it undrawn and hidden instead of erasing it, since the next frame paints over it; the dialogue box covers the top bands, which stop being pushed while it is open.

### Content is table-driven: `src/game/Levels.h`
A floor is a row of `LEVELS`: an ASCII map (`MAP_ROWS` = 14 rows, all the same length, at most `Stage::MAX_COLS`), the backdrop palette, the boss's sheet and the coworker's dialogue (pairs of lines, 24 chars max). The legend is in the comment above `Tile`; sprites stand on the bottom of their cell. Ground and cabinets are autotiled from their neighbours at load (`Stage::autotile`). The map never scrolls vertically: 14 rows are exactly the play area's height. `CIRCLES` names the floors still to build, for the results card.

### Assets
`src/assets/png/sprites/` holds the artist's PNGs and `src/assets/aseprite/` their sources. `tools/convert_sprites.py` turns `sheets/*.png` (frame sizes in its `SHEETS`) into indexed frames, each cropped to its opaque box with one palette per sheet, and slices `tiles/tileset.png` at the spots in its `TILES` table into 16x16 RGB565 tiles (the `TileArt` enum). Never edit `Sprites.{h,cpp}` by hand. `woman.png` (Bea) and `coffee.png` (the broken machine) are placeholders to be replaced by the artist at the same size. Facing: the player, printer and boss art face right; the walkers face left.

Sheet frames used by `Stage::collectDraws` — player: 0 idle, 16 mug raised, 11–13 run, 6–7 jump, 8 fall, 9 wall slide, 5 dash, 1 dead. Walker: 0–5 walk, 9 standing, 6–8 squashed. Boss: 0–1 the manager waiting, 2–5 turning into the mantis, 6–13 walk, 14–16 lean back, 17–19 dizzy.

### Colours
UI colours come from `src/ui/Theme.h` as `Color` slots resolved in the active palette at draw time, as in the other games. Level art (the backdrop, checkpoint flags, the dash trail) is plain RGB565 in `Levels.h`, `Renderer.cpp` and `Stage.cpp`.

### Persistence
`src/game/SaveGame.{h,cpp}` writes one `SaveBlob` of per-floor records (best time in steps, fewest deaths, clears) to NVS under the `decaf` namespace, in two alternating slots with a sequence number and checksum. It is written only when a floor is cleared. Settings live under their own key. Adding a field means bumping `SAVE_VERSION`; adding a floor changes the `layout` fingerprint; a mismatched save is discarded.

## Performance
The band renderer's inner loops run 70 000 pixels a frame: keep per-pixel work to table lookups and masks (the backdrop's patterns repeat every 128 px so `& 127` replaces a modulo). Avoid `String` in refresh paths: `char buf[N]` + `snprintf` + `setText(const char *)`.
