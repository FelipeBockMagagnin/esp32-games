# CLAUDE.md

This file provides guidance to Claude Code (claude.ai/code) when working with code in this repository.

## Project

Boba, a virtual pet in the manner of the 1996 keychain toys, for an ESP32 with a 240x320 ILI9341 SPI display, five push buttons and a passive buzzer (the same board as `games/idle`). PlatformIO + Arduino framework, C++. Its engine came from `games/snake`, its save and simulator from `games/tilde`. A 32x16 dot-matrix LCD, eight icons, a pet that hatches, calls, sulks, gets sick, sleeps by the clock and dies of neglect or old age. It is the opposite of Tilde on purpose: slow, one-bit and unforgiving.

The rules and the look are the genre's; the name, the characters and every sprite are our own. Do not copy the original's characters, use its trademark or its "-tchi" names, on the site or in the code. The family is invented creatures, not objects: solid bodies with a strong silhouette each, faces in holes.

## Code style

**Comment sparingly.** Write a comment only when the code cannot say it: a non-obvious constraint, a hardware quirk, a balance number's source, or why something that looks wrong is deliberate.

## Commands

`pio` is not on `PATH`; it lives at `~/.platformio/penv/bin/pio`.

```bash
pio run                              # Build
pio run -t upload -t monitor         # Flash and open the serial monitor (115200)
pio run -e dev -t upload -t monitor  # Dev build: screen, shot, form, hunger, happy, poop, sick, age, clock, theme, skip
~/.platformio/penv/bin/python tools/screenshot.py out.png  # Dev build on the board: the display as a PNG
tools/test.sh                        # Logic tests
tools/sim.sh [lives] [seed]          # Lives with eleven kinds of owner
tools/shots.sh [out_dir]             # The real screens drawn on the PC, one PNG per scripted scene
```

`tools/test.sh` and `tools/sim.sh` compile `src/pet/` against `tools/stub/` with `PET_TOOLS`. **Rerun both after touching `src/pet/`.** `tools/sim.sh` exits non-zero when an adult form is reachable by no owner. `tools/shots.sh` builds the screens against `tools/stub/TFT_eSPI.h` (fonts from `.pio/libdeps`, so it builds the firmware once if that is missing). **It is how to look at a change to the art or a layout without a board.** `tools/make_mockups.py` takes the site's `screen.png` from its `theme_Classic` scene.

## Hardware

- **All TFT_eSPI setup lives in the repo root's `common.ini` (`[board]`)**; only the GLCD font, used by Settings and the shell's wordmark.
- **Pins are `const int` at the top of `src/main.cpp`**: right 23, left 26, confirm 27, up 19, down 22, buzzer 32.
- **Backlight** (`TFT_BL`, 21) is PWM on LEDC channel 4 (`Backlight` in `lib/dotcore`), since `tone()` owns channel 0. A call from the pet wakes it, by reaching `Backlight::update()` as if it were a press.
- **No battery and no clock:** the board runs off USB and forgets the time at power-off. Hence the clock screen at every boot, and a pet that is simply frozen while off.
- Hold Up + Down while powering on to erase the save; Settings also has a reset.
- **The game menu:** when the site installed the launcher, Settings has a Game menu row (`returnToLauncher()`), and holding Confirm while powering on opens it.

## Controls

The original's A/B/C become Left/Right (walk the icons), Confirm (act) and Up (back). Down shows the clock. Holding Left 1.5 s opens Settings, from the pet or the grave; the press still moves the selection first, as Tilde's does.

## Architecture

**No blocking `delay()`.** Elapsed time uses unsigned subtraction, deadlines a signed difference.

- **`src/pet/`** is plain C++ with no Arduino or TFT include. `Pet` is the whole state (`Snapshot`), advanced by `tick()` once per powered second. Actions go through `check()`/`act()`, which refuse with a `Refusal`; `takeEvents()` returns `PetEvent` bits that `main.cpp` turns into sounds and hands to Home. Its dice are `Rng`, never Arduino's `random()`, so the simulator replays from a seed. The dice of a mere animation (the pet's wander) may be Arduino's.
- **Time only passes through `tick()`.** `setClock()` is never elapsed time: it moves the pet to where the new hour says (asleep with the light off, or awake) with no mistake, no ageing and no events. `updateTime()` in `main.cpp` holds the ticks while the clock screen is open.
- **`src/ui/LcdMatrix`** is a 32x16 bitmap; `present()` sends only the dots that changed, so screens simply redraw it whole. `Panel` paints the shell and the icons. Text on the matrix is a 3x5 font, 8 chars to a row: uppercase ASCII, digits and `: - / ? ! >`.
- **`src/art/Sprites.cpp`**: a form is one drawing, read at draw time. Its marks (see `Sprites.h`) give every pose: `a`/`b` the two frames of a step, `o`/`c` eyes open or shut, `m`/`n` the mouth, `j` what shows when it cheers. The forms are solid, so a face is holes that `c` and `n` fill in: an eye is a `c` row over an empty one, closing to a slit. Forms are at most 16x16 and stand on the matrix's floor. `FORM_ART` follows `FormId`'s order.
- **Screens:** `HomeScreen` is the toy and plays the scenes (`Scene`, each `SCENE_STEPS` long). `ClockScreen`, `GameScreen`, `SettingsScreen` and `ConfirmScreen` are modals; events are only passed to Home while it is showing, and it catches up in `onEnter()`.

### Rules: `src/pet/Config.h`

`FORMS` holds each form's decay rates, sleep hours, base weight and lifespan. Growth is in `Pet::updateGrowth` and waits for the pet to be awake. A call unanswered for `CALL_SECONDS` costs one mistake per need per call. A false call (from the child on, less often with discipline) refuses food and play until scolded, and an ignored one simply lapses. The child's mistakes pick the teen; the teen's mistakes and the discipline pick the adult; a flawless Zuzu becomes the Reizinho. Old age takes a pet on waking, so the owner sees it. Mistakes shorten life. While asleep nothing decays and the only call is the light.

From `tools/sim.sh 100`: looking every 10 minutes and scolding gives a Reizinho and about 29 years; without scolding a Babu and 19; every 2 to 3 hours a Babu and about 19; a careless first day then a good owner a Nhoca and 11; every 6 hours a Treco and 5; missing whole days usually ends in neglect.

### Colours

**Every colour comes from `src/ui/Theme.h`** as a `Color` slot: the ones `lib/dotui` draws with, then `FIELD`, `GHOST` (unlit dots and icons), `INK` and `SHELL`. `LCD_THEME` in `Themes.h` derives the dotui slots from those, so Settings looks like the same LCD. Nothing may rely on colour alone.

### Persistence

NVS namespace `boba`. `SlotStore` writes one `SaveBlob` (`pet/Save.h`), alternating two slots; `AutoSaver` writes every 60 s, brought forward to 5 s after an action or a pet event. Adding a field means bumping `SAVE_VERSION`, and a new form is caught by the layout fingerprint. A mismatched save is discarded. Settings live under their own key.
