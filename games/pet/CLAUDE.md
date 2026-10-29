# CLAUDE.md

This file provides guidance to Claude Code (claude.ai/code) when working with code in this repository.

## Project

A colony/farm game for an ESP32 with a 240x320 ILI9341 SPI display, five push buttons and a passive buzzer: the same board as `games/idle`, whose engine (UI, screens, saves, themes) this project started from. A habitat of tiles holds crops and critters: crops feed the critters, critters earn coins, coins clear the wild and buy buildings, seeds, species and upgrades. The world only runs while the board is powered; there is no offline progress.

## Code style

**Comment sparingly.** Write a comment only when the code cannot say it: a non-obvious constraint, a hardware quirk, a balance number's source, or why something that looks wrong is deliberate. Do not narrate what the next line does, restate a name, label sections, or explain a change in a comment (that goes in the commit). Most functions and members need none.

## Commands

`pio` is not on `PATH`; it lives at `~/.platformio/penv/bin/pio`.

```bash
pio run                       # Build
pio run -t upload -t monitor  # Flash and open the serial monitor (115200)
pio run -e dev -t upload -t monitor  # Dev build: serial cheats (coins, food, ripe, open, unlock, hungry)
tools/sim/run.sh [--days 2] [--seed 1] [--session 5] [--log]
tools/sim/test.sh             # Logic tests: save slots, new game, pathfinding, regrowth
```

The **balance simulator** compiles the real `src/game/` for the PC (against `tools/sim/stub/Arduino.h`) and plays a scripted player (active 30 min, then a 5-minute check-in every waking hour, asleep 00:00-08:00), printing an hourly line (population, coins, income, food, fields) and each first adoption, seed and upgrade, and when the whole map is cleared. **Rerun it after touching a table in `WorldConfig.h`**; its current arc: Lumifox on day 2, the map cleared around day 4. Departures late in a run mean the colony outgrew its fields.

## Hardware

- **All TFT_eSPI setup lives in the repo root's `common.ini` (`[board]`), shared by every game**, not a `User_Setup.h`. Only the GLCD font (font 1) is loaded; enabling another costs flash.
- **Pins are `const int` at the top of `src/main.cpp`**, the same as the idle game's board. Buttons are `INPUT_PULLUP`.
- **D-pad:** right (23), left (26), confirm (27, red cap), up (19), down (22). Right/Left step through the screens, **except on the Habitat**, where every arrow moves the cursor and Left/Right leave only from the grid's edge.
- **Global gestures** (in `main.cpp`): hold Left 1.5 s opens Settings (not on the Habitat, where a held Left sweeps the cursor); hold Up + Down 3 s opens reset; Up + Down while powering on erases the save.
- **Backlight** (`TFT_BL`, 21) is PWM on LEDC channel 4, since `tone()` owns channel 0 and its timer.

## Architecture

### Main loop
`src/main.cpp` owns every object as a global and wires them. **No blocking `delay()` in game code**: every periodic component takes `now = millis()` and self-throttles. `World::update()` and `Colony::update()` run every loop whatever screen is visible, each on a fixed `TICK_MS` tick pushed from `now` (`deadline = now + interval`), so a stalled loop loses time rather than bursting. Elapsed time uses unsigned subtraction, deadlines a signed difference, so both survive `millis()` rollover.

**Global declaration order matters:** `World` → `Colony` (the colony holds a reference to the world).

### Screens
`ScreenManager` cycles Habitat → Colony → Shop. `BuildMenuScreen` (opened by Confirm on a grass, soil, crop or built tile), `SettingsScreen`, the reset `ConfirmScreen` and the boot `SplashScreen` are modals.

A `Screen` subclass declares its `UI` elements as **members**, registers them once with `addElement(&member)` and `setHeader(&header)`, then:
- `update(now)` pushes fresh values, gated by `if (!refreshDue(now)) return;`; input handlers call `forceRefresh()`;
- `onConfirmPress`/`onUpPress`/`onDownPress` handle input; `onBackPress` and `onNextPress` return **true to consume** Left/Right;
- `repeatsArrows()` makes a held arrow repeat through `onArrowRepeat(Arrow)` (only the Habitat), which returns false at an edge so a repeat never leaves the screen;
- `onEnter(tft)` resets screen-local state, then calls `Screen::onEnter(tft)`.

Nav names stay at 9 chars or less. Births and departures reach the header of whichever screen is open through `applyNotice`, from `Colony::getEventCount()` diffed in `main.cpp`.

### Rendering: dirty flags, no full redraws
`src/ui/UI.h` is the core. Nothing is cleared per frame; each element tracks `dirty` and the bounds of its last draw. **Every mutator is a no-op on an unchanged value.** Elements are never added or removed at runtime; lists keep a fixed `ListRow rows[N]` and refill them as `ListView` scrolls.

**The habitat grid** (`src/screens/habitat/TileGrid`) composites each tile whole (ground, up to two critters, the cursor) into one 16x16 `TFT_eSprite` and pushes it, so overlapping never leaves pixels behind. It repaints only the tiles `World` flagged (`markChanged`/`takeChanged`): anything that changes how a tile looks must mark it, including a critter moving (`Colony::moveTo` marks both tiles) or a pond appearing (`markPondArea`). Tile and critter art is drawn with primitives in `TileArt.cpp`, not stored, because flash is the limit on art.

### Colours
**Every UI colour comes from `src/ui/Theme.h`** as a `Color` slot, resolved in the active palette (`THEMES` in `src/ui/Themes.h`) at draw time, so a stored colour must be a `Color`, never a `uint16_t`. All themes are open. Terrain, crop and critter colours are art, in `WorldConfig.h` (`TerrainArt`, `CROPS`, `SPECIES`).

### Game state
- **`World`**: the tiles (kind, crop, growth, hand-watering left), coins (whole plus thousandths), the pantry, cleared-tile count, unlocked crops and upgrades. A crop grows only while watered, by hand or from a pond in range; a harvest leaves it to **regrow from a sprout**, so seeds are paid once. `findPath` is a breadth-first search over walkable ground (everything but wild and ponds) to the nearest tile of a kind.
- **`Colony`**: up to `MAX_CRITTERS` critters with hunger, energy and a state machine (wander, go eat, eat, go to bed, sleep, gather). Idle critters **gather ripe crops** into the pantry, which is what lets the colony run between check-ins. Mood (flowers nearby, a bed, hunger) scales income and gates breeding; starving `LEAVE_SECONDS` makes a critter leave. A full colony progresses by releasing weak critters (Confirm twice on the Colony screen) for better species.

### Persistence
`src/game/SaveGame.{h,cpp}` writes one raw `SaveBlob` to NVS, alternating between two slots that each carry a sequence number and a checksum, so a power cut mid-write only loses that write. Settings live under their own `settings` key. **Snapshots stay POD: primitives and indices into the config tables.** Adding a field means bumping `SAVE_VERSION`; adding a table row is caught by the `layout` fingerprint. A mismatched save is discarded. A zeroed snapshot is a new game (`World::load` sees an all-wild grid).

`AutoSaver` writes every 60 s, brought forward to 5 s when the player acts (`World::getRevision`, `Colony::getRevision`), and skips byte-identical blobs. The blob is about 1.6 KB, so the interval is a flash-wear budget: don't shorten it, and don't bump a revision for what critters do on their own (gathering), only for the player's actions and colony changes.

### Content is table-driven
`src/game/WorldConfig.h` holds `BUILDS` (`BuildId` must stay in table order), `CROPS` (at most 8, a byte of flags), `SPECIES`, `UPGRADES` (at most 16) and `CRITTER_NAMES`, with `*_COUNT` derived via `sizeof`. Build prices compound by `growthPercent` per one already built; clearing tile n costs `CLEAR_BASE + n^3 / CLEAR_DIVISOR`. A new `UpgradeEffect` needs a place that reads `World::getEffect`. A new `CritterShape` needs a case in `TileArt::paintCritter`. Name length limits are in each table's field comments.

## Performance
Avoid `String` concatenation in refresh paths: use `char buf[N]` + `snprintf` + `setText(const char *)`.
