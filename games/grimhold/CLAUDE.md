# CLAUDE.md

This file provides guidance to Claude Code (claude.ai/code) when working with code in this repository.

## Project

Grimhold, a dark-fantasy first-person dungeon crawler drawn by a raycaster, for an ESP32 with a 240x320 ILI9341 SPI display, five push buttons and a passive buzzer (the same board as `games/idle`). PlatformIO + Arduino framework, C++. A roguelite: every floor is generated from the run's seed, the player strikes with a sword and charges spells, gear picked up is worn at once when it beats what is worn, every fifth floor a lich guards a sealed portal, and death ends the run (only the records stay). The menus' engine came from `games/snake`, the landscape layout from `games/decaf`, the floor generator from `games/deepwell`.

## Code style

**Comment sparingly.** Write a comment only when the code cannot say it: a non-obvious constraint, a hardware quirk, a balance number's source, or why something that looks wrong is deliberate. Do not narrate what the next line does, restate a name, label sections, or explain a change in a comment (that goes in the commit).

## Commands

`pio` is not on `PATH`; it lives at `~/.platformio/penv/bin/pio`.

```bash
pio run                              # Build
pio run -t upload -t monitor         # Flash and open the serial monitor (115200)
pio run -e dev -t upload -t monitor  # Dev build: serial commands fps, next, shot
~/.platformio/penv/bin/python tools/screenshot.py out.png  # Dev build on the board: the display as a PNG
tools/test.sh                        # Logic tests: 2000 seeds of floors, collision, blows, spells, gear, the save, the tutorial
tools/shots.sh [out_dir]             # The real screens drawn on the PC, one PNG per scripted scene
python3 tools/make_placeholders.py   # Redraws the placeholder art and copies the idle game's into art/
python3 tools/convert_art.py         # art/ -> src/render/Art.{h,cpp}
```

`tools/test.sh` compiles `src/crawl/` and `lib/dotcore`'s `SlotStore.cpp` against `tools/stub/`. **Rerun it after touching `src/crawl/`.** `tools/shots.sh` builds the screens and the renderer against `tools/stub/TFT_eSPI.h` (fonts from `.pio/libdeps`, so it builds the firmware once if that is missing); `tools/make_mockups.py` takes the site's `screen.png` from its `04_spell` scene, turned upright.

## Hardware

- **All TFT_eSPI setup lives in the repo root's `common.ini` (`[board]`)**; only the GLCD font.
- **Landscape, like Decaf.** The board is held on its side, a quarter turn clockwise, so the D-pad is under the left thumb. `ROTATION` and the pins at the top of `src/main.cpp` are named for the way each button points held like that. `-DDISPLAY_LANDSCAPE=1` lays the menus out 320x240. The env builds with `-O2` instead of `-Os`, for the raycaster's inner loops.
- **Controls in a run:** Up walks forward, Down back, Left and Right turn. A tap of Confirm (released within `TAP_MS`) swings the sword; held longer it charges a spell that leaves on release, stronger the longer it was held, a full charge bursting over everything near where it lands. Up and Down together pause. Buttons debounce at 15 ms, and a Confirm press shorter than a frame still reaches the run as one frame held.
- **Backlight** (`TFT_BL`, 21) is PWM on LEDC channel 4 (`Backlight` in `lib/dotcore`), since `tone()` owns channel 0.
- Hold Up + Down while powering on to erase the save; Settings also has a reset.
- **The game menu:** when the site installed the launcher, Settings has a Game menu row (`returnToLauncher()`), and holding Confirm while powering on opens it.

## Architecture

**No blocking `delay()`.** Elapsed time uses unsigned subtraction (`now - last`), deadlines a signed difference.

### Layers
- **`src/crawl/`** is the game, plain C++ with no Arduino or TFT include. `Run` is the whole state, advanced by `update(input, dt)` (dt capped at 50 ms) and reporting what happened as `Event` bits through `takeEvents()`, which `PlayScreen` turns into sounds and notices. `Map` carves rooms joined in order by L-shaped corridors on a 32x32 grid, pillars at the big rooms' inner corners (off the corridors' lines), the portal in the middle of the room farthest from the start; a floor is regenerated from `seed` and the floor number. Enemies chase by line of sight (`Map::clearLine`), wind up, strike or shoot, rest; a blow staggers and pushes them back, the boss never. `Inventory::pickUp` wears an item only when its value beats the worn one of its slot; the loser goes to the bag, or back to the floor when the bag is full, marked to wait until the player steps off it. Every random roll is the run's own `Rng` (xorshift), never Arduino's `random()`. `Tutorial` teaches the controls in the first floor's starting room, one hint at a time (walk, turn, sword, spell, pause), each waiting for the player to do it; leaving the room ends it as well.
- **`src/render/`** draws the view. `View` is plain C++ too, so the PC shots draw the same pixels: `prepare()` casts 160 rays (each two pixels wide), lays out every sprite (enemies, items, potions, spells, sparks) sorted far to near and the overlays (the sword, the casting hand and its orb), and `drawBand()` fills rows of byte-swapped RGB565: ceiling and floor as a shaded gradient, textured walls, sprites clipped by each column's wall distance, the overlays, the sight, the boss's bar and the red edge of a hit. `Frame` sends the 320x192 view in six 32-row bands by DMA, drawing one while the last goes out, from two 20 KB buffers.
- **`src/render/Art.{h,cpp}` is generated** by `tools/convert_art.py` from `art/`: every picture is up to 63 colours, texels a column at a time, index 0 transparent, with its palette precomputed at `SHADES` distance levels (torchlight: blue fades first) plus a hit-flash row, so a texel goes to the buffer without arithmetic. The menus' 16x16 item icons come from the same PNGs. **Never edit it by hand; change `art/` and rerun the converter.**
- **`art/`** holds the source PNGs. `tools/make_placeholders.py` copies the idle game's (seven enemies, two backgrounds cut into wall textures, the item icons in series of five tiers) and draws placeholders for the rest (crypt, bone and blood walls, the portal and its sealed face, skeleton, wraith, lich, the spells, the sword and the hand); a hand-drawn PNG of the same name and size replaces one. The converter's lists fix the order of the walls, enemies, effects and hands, which `Config.h`'s `EnemyKind` follows.
- **`src/core/`**: `ScreenManager` (its `update()` renders only if the screen did not open another modal meanwhile) and `SoundManager`.

### Screens
The rotation is the title alone; everything else is a modal. `PlayScreen` takes every arrow and reads the buttons held (`setHeld`), ignoring them until the ones that opened it are let go. Pausing swaps it for `PauseScreen` (resume, inventory, potions, settings, abandon), which leads to `InventoryScreen` (worn on the left, the bag on the right, Confirm wears, Right drops at the player's feet) and back; `SettingsScreen` and `ConfirmScreen` return to whichever modal `setReturnTo` names. Death waits `DEATH_PAUSE_MS` on the last frame, then `GameOverScreen`. The tutorial's hint takes the HUD's notice line whenever no other notice is showing. The HUD under the view is `lib/dotui` elements, which redraw only what changed; the boss's bar is drawn inside the view, since a HUD element sharing space with others could not erase on its own.

### Content is table-driven: `src/crawl/Config.h`
`ENEMIES` (health, damage, speed, reach, ranged, windup and rest, size on screen, first floor), the growth per floor, `SLOTS` (what each slot's value means, its base, per tier and per floor, its nouns), the tiers' names, the wall sets per band of floors, the player's speeds and the blow's and spell's numbers. A new enemy needs a row here, a PNG in `art/enemies/` and its name in `convert_art.py`'s `ENEMIES`, in the same place.

### Colours
**Every menu colour comes from `src/ui/Theme.h`** as a `Color` slot: the ones `lib/dotui` draws with (XP_* is the mana bar), then HP_* and the four tiers above the first (`Theme::tierColor`). Three themes in `Themes.h`. The 3D view's colours are art: the textures and sprites, and the ceilings, floors, boss bar and hit edge in `View.cpp`.

### Persistence
NVS namespace `grimhold`. One `SaveBlob` (`crawl/Save.h`: the run as it stood when its floor began, and the records, which also say whether the tutorial ended, so it runs once per save) through `SlotStore`, written when a run starts, at every new floor, on death and on abandon; a dead or abandoned run is saved as no run. A power cut mid-floor resumes at the start of that floor. Settings live under their own key. Adding a field means bumping `SAVE_VERSION`; a table changing size is caught by the layout fingerprint.
