# CLAUDE.md

This file provides guidance to Claude Code (claude.ai/code) when working with code in this repository.

## Project

Fluid, a physics toy for an ESP32 with a 240x320 ILI9341 SPI display, five push buttons and a passive buzzer (the same board as `games/idle`). PlatformIO + Arduino framework, C++. Its engine (core, settings, saves) came from `games/snake`. Two modes: **Ink**, a grid fluid the arrows stir with a nozzle that lays dye, and **Water**, particles in a box the arrows tilt. Six palettes colour the field; four themes colour the menus. Original code, not a port: the research list's MIT reference (colonelwatch/ESP32-fluid-simulation) was only the idea.

## Code style

**Comment sparingly.** Write a comment only when the code cannot say it: a non-obvious constraint, a hardware quirk, a tuned constant's source, or why something that looks wrong is deliberate. Do not narrate what the next line does, restate a name, label sections, or explain a change in a comment (that goes in the commit).

## Commands

`pio` is not on `PATH`; it lives at `~/.platformio/penv/bin/pio`.

```bash
pio run                              # Build
pio run -t upload -t monitor         # Flash and open the serial monitor (115200)
pio run -e dev -t upload -t monitor  # Dev build: serial commands screen <i>, shot; prints the field's frame times
~/.platformio/penv/bin/python tools/screenshot.py out.png [--screen 1]  # Dev build on the board: the display as a PNG
tools/test.sh                        # Simulation tests: ink divergence and bounds, water settling, volume, tilts, the renderer, the save
tools/shots.sh [out_dir]             # The real screens drawn on the PC, one PNG per scripted scene
```

`tools/test.sh` compiles `src/fluid/` and `lib/dotcore`'s `SlotStore.cpp` against `tools/stub/`. Its water checks (surface height, densest particle under 1.4x rest, settled speed) are what the constants in `WaterSim.cpp` were tuned against. **Rerun it after touching `src/fluid/`.** `tools/shots.sh` builds the screens against `tools/stub/TFT_eSPI.h` (fonts from `.pio/libdeps`, so it builds the firmware once if that is missing) and drives the nozzle with held arrows; `tools/make_mockups.py` takes the site's `screen.png` from its `04_ink_swirls` scene. Look at the shots after changing a simulation or the renderer: most of the tuning is visual.

## Hardware

- **All TFT_eSPI setup lives in the repo root's `common.ini` (`[board]`)**; only the GLCD font.
- **Pins are `const int` at the top of `src/main.cpp`**: right 23, left 26, confirm 27, up 19, down 22, buzzer 32.
- **Backlight** (`TFT_BL`, 21) is PWM on LEDC channel 4 (`Backlight` in `lib/dotcore`), since `tone()` owns channel 0.
- **DMA:** `main.cpp` calls `initDMA()` and hands the result to `PlayScreen`, which falls back to blocking `pushImage` without it.
- Hold Up + Down while powering on to erase the save; Settings also has a reset.
- **The game menu:** when the site installed the launcher, Settings has a Game menu row (`returnToLauncher()`), and holding Confirm while powering on opens it.

## Architecture

**No blocking `delay()`.** Elapsed time uses unsigned subtraction (`now - last`).

### `src/fluid/`: plain C++, no Arduino or TFT include
- **`InkSim`**: Stam's stable fluids on 48x64 cells (5x5 px each) with a ring of wall cells. A step is vorticity confinement, one pressure projection (Gauss-Seidel with over-relaxation, warm-started from the last step's pressure), then semi-Lagrangian advection of the velocity and the three dye channels. Velocity is in cells per step, dye 0..1 per channel. The cells along the walls fade faster, or dye pools there in a bright frame. Nine float arrays, about 120 KB of heap.
- **`WaterSim`**: 600 particles in the 240x320 px box, by Clavet's double density relaxation plus its linear viscosity (which stills the surface's jitter), on a uniform grid of 20 px cells rebuilt by counting sort every step. Velocities come back out of the positions, so the walls only clamp. Two steps a frame.
- **`FieldRender`**: draws a band of rows in panel byte order (RGB565 high byte first). Both fields are stretched over the screen by bilinear interpolation: the ink's dye straight onto the palette's background, with a gain and a 2x2 dither; the water by splatting the particles onto a 60x80 grid of density and speed, then a 64x8 colour table by density (the threshold that turns the particles into one body, a lit rim along the surface, the deep end darker) and speed (foam).
- **`Palettes.h`**: per palette a background, four inks the nozzle cycles through, and the water's body, rim and foam.
- **`Prefs`**: the last mode and palette, through `SlotStore`; adding a field means bumping `PREFS_VERSION`.

### Screens and input
A loop of Title (Ink, Water, Palette, Theme, a banner of the palette's inks rolling as waves) ↔ Settings (sound, dimming, game menu, reset). `PlayScreen` is a modal opened from the title in the chosen mode. It takes the whole panel and repaints it every 33 ms from `update()`, in 16-row bands, each drawn while the last goes out by DMA, so it has no elements; it takes its memory on entering and frees it on leaving (and shows "Out of memory" if the heap is short).
- **Ink:** `main.cpp` passes the arrows held each frame (`setHeld`); while any is, the nozzle speeds up that way, pushes the fluid and lays the palette's ink, advancing through the four stops. Two arrows held move it diagonally; let go, it coasts to a stop.
- **Water:** an arrow turns gravity that way; pressing the way it already falls throws the water back up (a shake).
- **Confirm** freezes the field under a menu box: Resume, Clear (Refill), the other mode, the palette (Left/Right or Confirm change it, repainting the field behind), Quit.

### Colours
**Every menu colour comes from `src/ui/Theme.h`** as a `Color` slot, only the ones `lib/dotui` draws with; the field's colours come from the palettes, never the theme.

### Persistence
NVS namespace `fluid`. Prefs are written when the mode or palette changes; settings live under their own key.
