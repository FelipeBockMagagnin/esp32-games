# CLAUDE.md

This file provides guidance to Claude Code (claude.ai/code) when working with code in this repository.

## Project

NaN Idle, an idle RPG in the energy-allocation genre, ported from the Vue game in `~/Projects/nan_idle` (its sprites and its formulas, not its code) for an ESP32 with a 240x320 ILI9341 SPI display, five push buttons and a passive buzzer. PlatformIO + Arduino framework, C++. Energy is allocated into training skills, which raise attack and defense; those climb an endless boss ladder for XP, spent on the energy; the adventure zones drop gear and gold; gold buys miners and pays the augments' levels; a rebirth trades the run for NaN points. Every screen is a maximized Windows window over a taskbar, in the 98, XP or 7 style.

The web game's gaps are fixed here, not ported: one set of stats serves the boss and the adventure, gear and augments apply, the idle attack really attacks, every XP upgrade does something, training keeps its overflow, and the save holds everything. Engine pieces came from `games/mines` (core, screens, settings, stubs) and `games/idle` (`EnemySprite`, the sprite converter, the sprites themselves).

## Code style

**Comment sparingly.** Write a comment only when the code cannot say it: a non-obvious constraint, a hardware quirk, a balance number's source, or why something that looks wrong is deliberate.

## Commands

`pio` is not on `PATH`; it lives at `~/.platformio/penv/bin/pio`.

```bash
pio run                              # Build
pio run -t upload -t monitor         # Flash and open the serial monitor (115200)
pio run -e dev -t upload -t monitor  # Dev build: screen <i>, shot, xp <n>, gold <n>, boss <n>, level <n>
~/.platformio/penv/bin/python tools/screenshot.py out.png [--screen 1]  # Dev build on the board: the display as a PNG
tools/test.sh                        # Logic tests: energy, training, bosses, items, shops, adventure, rebirth, the save
tools/sim.sh [--hours 48] [--every 60] [--seed 1]  # Balance: when each feature, boss, zone and rebirth comes
tools/shots.sh [out_dir]             # The real screens drawn on the PC, one PNG per scene, every theme
python3 tools/convert_sprites.py     # Regenerates src/assets/Sprites.{h,cpp} from src/assets/png/sprites/
```

`tools/test.sh` compiles `src/nan/` and `lib/dotcore`'s `SlotStore.cpp` against `tools/stub/`; **rerun it after touching `src/nan/`**. `tools/sim.sh` plays the real `src/nan/` with a scripted player checking in every minute (allocates everything, buys the cheapest XP upgrade and the best miner, wears the best gear, fights when healed, farms the hardest zone it survives, rebirths after two hours without a boss); **rerun it after touching any number in `Config.h`**. Current pacing: Boss at ~1 min, XP ~6 min, adventure ~30 min, augments ~2 h, rebirth open ~4.5 h, a first rebirth ~10 h. `tools/shots.sh` needs the firmware built once (fonts from `.pio/libdeps`); `tools/make_mockups.py` takes the site's `screen.png` from its `04_adventure_xp` scene.

## Hardware

- **All TFT_eSPI setup lives in the repo root's `common.ini` (`[board]`)**; only the GLCD font, which has no accents, so the Portuguese text is written without them.
- **Pins are `const int` at the top of `src/main.cpp`**: right 23, left 26, confirm 27, up 19, down 22, buzzer 32.
- **Backlight** (`TFT_BL`, 21) is PWM on LEDC channel 4 (`Backlight` in `lib/dotcore`), since `tone()` owns channel 0.
- Hold Up + Down while powering on to erase the save; the Control Panel also has a reset.
- **The game menu:** when the site installed the launcher, the Control Panel has a Menu de jogos row (saves, then `returnToLauncher()`), and holding Confirm while powering on opens it.

## Controls

Left/Right walk the rotation (Treino, Boss, XP, Aventura, Itens, Minerar, Aprimorar, Renascer, Config), skipping what is still locked. Up/Down move the selection; held, they repeat, and so does Confirm where repeating makes sense (allocating, buying). On Treino and Aprimorar the first row picks what Confirm does to a row (+step, +half the idle energy, +all of it, -step, -all); the step is 5% of the cap. On Aventura Up/Down change zone and Confirm strikes. On Itens Confirm opens the owned items for that place, Left goes back.

## Architecture

**No blocking `delay()`.** Elapsed time uses unsigned subtraction, deadlines a signed difference. `main.cpp` ticks the game in fixed 100 ms steps whatever screen is showing, catching up at most ten steps after a stall.

- **`src/nan/`** is plain C++ with no Arduino or TFT include. `Game` holds every rule and number; `Config.h` every table. Numbers are `double` and grow without bound; `Format` prints them (`12.3K`, `4.56Qa`, then `1.2e48`). Screens only see the game through counters (`bossKills()`, `advKills()`, `drops()`, `unlockEvents()`, `playerSwings()`...) that they diff against what they last saw, resynced in `onEnter`.
- **Stats:** attack = (100 + Σ statPerLevel·level^1.3 of the attack skills) × gear × augments × rebirth, defense likewise; max HP is 10× attack, HP regen defense/20 a second (the web game's formulas). A rise in max HP raises both HP pools with it.
- **Combat:** the boss fight is continuous, `attack - defense` a second each way with no floor (the web game's rule), so a boss whose defense matches the attack cannot be hurt. The adventure is in swings, `attack - defense/2` (the web game's adventure rule), an auto-attack every second, a Confirm strike for 3× on a 3 s cooldown, the zone's boss every tenth kill. Dying in a zone sends the player to the safe zone, which heals 5×. Enemy stats are multiples of the zone's reference boss, so retuning the ladder carries the zones along.
- **Bosses** are generated: attack `5e4 × 4^i` (the web game's first boss), defense 0.8×, HP 10×, XP `2 × 1.5^i`. `BOSSES` names and draws them, repeating with `+N` past its end.
- **Unlocks** are a bit per `Feature`, saved, and kept by a rebirth; `main.cpp` locks each screen until its feature is open and pops a message box, whose OK goes to it.
- **Rebirth** keeps the XP upgrades, items, unlocks and NaN points and clears the rest (`Game::clearRun`).

### The Windows look
`src/ui/Win.{h,cpp}` draws everything Windows: the window frame and title bar, bevels, fields, buttons, progress bars, focus frames and the taskbar, each three ways after the active theme's `WinStyle` (CLASSIC square bevels and block bars, LUNA rounded blue title and green blocks, AERO pale glass and smooth bars). Every screen passes its title to `Screen`, which registers its `WinFrame` first; the `Taskbar` is one object shared by every screen (`Screen::taskbar()`), added last with `addTaskbar()`, showing the neighbouring screens, the play time, and notices (a boss beaten, a drop) flashing in place of the task buttons. A dialog passes no title and draws its own box over the desktop. Selection is a `FocusFrame` drawn round a row (dotted in 98) and erased by drawing it in the face colour, so rows never repaint for a selection change. Texts are opaque (`ui/Labels.h`) so changing numbers overwrite without an erase.

### Colours
**Every colour comes from `src/ui/Theme.h`** as a `Color` slot: the ones `lib/dotui` draws with, then the chrome's (`DESKTOP`, `FACE`, `TITLE_A/B`, `EDGE_*`, `FIELD`, `HILITE`, `BAR`, `HP`, `TASKBAR`, `START`, `CLOSE`, `TRAY`...). A new slot needs a column in every `THEMES` row. Art is the exception: the sprites' palettes, the Windows flag and the boot screen's black, and the strike numbers' yellow. Adding Windows 10/11 is a `WinStyle` (flat) and two `THEMES` rows.

### Assets
`tools/convert_sprites.py` turns `src/assets/png/sprites/` into `Sprites.{h,cpp}` (never edit those): enemies and backdrops as run-length palettes drawn 2× by `EnemySprite`, items cropped and box-filtered to 16x16 icons, icons (picked from `games/idle`'s library) as they are. `forest.png` is the tutorial backdrop recoloured; the rest are the artist's. `ui/Art.cpp` maps the model's `Art`, `Backdrop` and `ItemArt` to them, in enum order.

### Persistence
NVS namespace `nanidle`. `Game::Snapshot` is POD (numbers and table indices) and must fit one `SlotStore` record (256 bytes, `static_assert`ed; 176 now). Adding a field means bumping `SNAPSHOT_VERSION`; a mismatched or out-of-range save starts a new game. Saved every 60 s, brought forward to 5 s after a milestone (`Game::milestones()`), skipping unchanged bytes. Settings live under their own key. No offline progress: the board has no clock across power-off.
