# CLAUDE.md

This file provides guidance to Claude Code (claude.ai/code) when working with code in this repository.

## Project

*~tilde~*, a cozy virtual pet for an ESP32 with a 240x320 ILI9341 SPI display, five push buttons and a passive buzzer: the same board as `games/idle`, whose engine (screens, buttons, saves, themes) this project started from, by way of `games/deepwell`. The pet is a *glyph*: a soft, round kawaii creature drawn from anti-aliased shapes, in a room that moves a little all the time. It hatches, grows through eight forms chosen by how it is cared for, and never dies or leaves: neglect only makes it sad or sick, and one action mends each trouble. It lives only while the board is powered; there is no clock across power-off. The full scope and art direction are in `docs/DESIGN.md`.

## Code style

**Comment sparingly.** Write a comment only when the code cannot say it: a non-obvious constraint, a hardware quirk, a tuning number's source, or why something that looks wrong is deliberate. Do not narrate what the next line does, restate a name, label sections, or explain a change in a comment (that goes in the commit). Most functions and members need none.

## Commands

`pio` is not on `PATH`; it lives at `~/.platformio/penv/bin/pio`.

```bash
pio run                              # Build
pio run -t upload -t monitor         # Flash and open the serial monitor (115200)
pio run -e dev -t upload -t monitor  # Dev build: age <s>, need <i> <v>, bond <n>, form <i>, sick, fluff, screen <i>, theme <i>, fps, shot
~/.platformio/penv/bin/python tools/screenshot.py out.png [--screen 2]  # Dev build on the board: the display as a PNG
tools/test.sh                        # Tests: the rasterizer, forms, text, growth, the cozy promise, save slots
tools/sim.sh [days] [seed]           # A pet's first days with five kinds of owner
tools/shots.sh [out_dir]             # Every screen and animation rendered on the PC, as PNGs and GIFs
python3 tools/make_font.py           # Bakes tools/fonts/Nunito.ttf into src/gfx/FontData.cpp
```

The PC tools compile the real sources against `tools/stub/` (`Arduino.h`, an in-memory `Preferences`, and a `TFT_eSPI` that is a framebuffer). **`tools/shots.sh` is how to look at a change to the art, an animation or a layout without a board**: it drives the real screens through scripted scenes (hatching, each form, care, night, sickness, growing up, the minigames, every theme) and writes what the panel would show, a GIF for each animated scene. **Rerun `tools/sim.sh` after touching `src/pet/Config.h`**; the arc in that file's top comment comes from it. It checks that every adult form is reachable by some owner and how often a pet is sick or sad with each.

## Hardware

- **All TFT_eSPI setup lives in the repo root's `common.ini` (`[board]`), shared by every game**, not a `User_Setup.h`. The game draws nothing through TFT_eSPI but `pushImageDMA`: every pixel comes from `src/gfx/Canvas`.
- **Pins are `const int` at the top of `src/main.cpp`**, the same as the idle game's: right 23, left 26, confirm 27, up 19, down 22, buzzer 32. Portrait, rotation 2.
- **Gestures** (`main.cpp`): hold Left 1.5 s for Settings, Up + Down 3 s for the reset question, both off while a screen's `allowsGestures()` is false (a minigame, naming). Up + Down while powering on erases the save.
- **Backlight** (`TFT_BL`, 21) is PWM on LEDC channel 4 (`Backlight` in `lib/dotcore`), since `tone()` owns channel 0 and its timer. `tone()` queues its notes, so a jingle never blocks.

## Architecture

**No blocking `delay()` in game code.** Elapsed time uses unsigned subtraction, deadlines a signed difference (`(long)(now - deadline) >= 0`), pushed from `now`.

### Layers
- **`src/pet/`** is the game, plain C++ with no Arduino or TFT include. `Pet` is the whole state, advanced by `tick()` once per powered second; actions go through `check()`/`act()`, which refuse with a `Refusal` rather than doing nothing; `takeEvents()` returns `PetEvent` bits that `main.cpp` turns into sounds and notices. `Album` is what outlives a pet (forms seen, past pets, minigame records). Its dice are `Rng` (xorshift), never Arduino's `random()`, so the simulator replays from a seed.
- **`src/gfx/`** draws. `Canvas` is our own rasterizer, plain C++ so the PC tools draw the same pixels: anti-aliased circles, ellipses, rounded rects, round-ended lines, arcs, rings and triangles, each pixel's cover from the shape's signed distance; convex shapes fill their inside a row span at a time and pay for the distance only near the edge (`spans`, checked against the exact cover in `tools/test.sh`). Text is Nunito baked to 4-bit alpha (`Font.h`, printable ASCII only); symbols are vector `Icons`. `Display` sends a frame as ten 32-row bands by DMA, drawing the next band while the last goes out, and skips a band whose checksum did not change. `Widgets` are the shared card, pill meter, round button and hint.
- **`src/art/`**: the pet and its room. `Rig` defines each form as shapes (`FORM_RIG`: body size and colour, eyes, its one feature) and draws a `Pose` (squash and stretch, lift, lean, where it looks, eyes, mouth, blush, pout, pale, flash). `PetView` animates it on the Home screen: a damped spring for squash and stretch, jumps with a crouch and gravity, behaviours picked by mood (walk, sit, look around, yawn, dance, bounce, nose the plant, chase the dust, sigh, doze), reactions (looks at the picked button, cheers when Home opens, pouts at a refusal, startles at new fluff, stretches awake), the care animations and vector `Particles`. `Room` is the living room: drifting clouds and a bird in the window, a moon and twinkling stars while it sleeps, a lamp that flickers on, a swaying plant, dust in the sunbeam, and the room fading to night with the lamp off. `idlePose` shows the pet alive on the other screens.
- **`src/core/`** is game-agnostic: `ScreenManager` and sound. Buttons, backlight, settings and `SlotStore` live in the repo's `lib/dotcore`.

### The visual rules
- **Kawaii and soft**: round shapes with a soft outline a shade darker than their fill, a highlight and a shadow; pastel cards with a drop shadow; nothing square, nothing black.
- **Every colour is a slot** of `src/gfx/Theme.h`, resolved in the active palette (`THEMES` in `Themes.h`) at draw time; shades come from `Canvas::mix`. A new slot needs a column in every theme.
- **A form is data, not frames**: a row of `FORM_RIG` and, for a new feature, a `case` in `Rig.cpp`'s `feature()`. Every face works on every form. `tools/test.sh` checks that each form fits the room at its biggest squash and jump, and draws the same twice.
- **Motion is springs and easing**: a change of state kicks a spring or eases in, never snaps; one-off effects are particles, never extra state in a screen.
- Text is printable ASCII (the baked font's); write symbols with `Icons`. Cozy wording: the pet is never said to die, leave or rest forever; a past pet "keeps a page" in the Album.

### Content is table-driven: `src/pet/Config.h`
`FORMS` (`FormId` and `FORM_RIG` in `art/Rig.cpp` follow its order), `NEEDS` (seconds per point awake and asleep), the stage lengths, the care amounts and the bond thresholds. The growth rules are in `Pet::tickGrowth`: a baby played with (minigames) more than fed becomes a Tuft, otherwise a Mochi; a child becomes the close or the shy adult of its line by `ADULT_HIGH_BOND`. Flavour text for the speech bubble is in `src/screens/Lines.h`, 210 px at most in `FONT_SMALL` with an 8-letter name.

### Persistence
`SlotStore` writes one `SaveBlob` (`pet/Save.h`: the pet's `Snapshot` and the `Album`) to NVS under the `tilde` namespace, alternating two slots with a sequence number and a checksum, so a power cut mid-write only loses that write. `AutoSaver` writes every 60 s, brought forward to 5 s after care or a pet event; the interval is a flash-wear budget. Settings live under their own key. Snapshots stay POD; adding a field means bumping `SAVE_VERSION`, and a new form or need is caught by the layout fingerprint. A mismatched save is discarded.

## Performance
A frame is drawn every 33 ms whole (no dirty tracking), so drawing cost is the budget: the dev build's `fps` prints a frame's drawing time and how many bands it sent. Big shapes are cheap (their insides are spans); what costs is many pixels of edge and large semi-transparent shapes. Skip a group early with `Canvas::reaches()` when its rows miss the band. The two band buffers are 30 KB of RAM.
