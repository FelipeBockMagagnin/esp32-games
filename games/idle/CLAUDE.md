# CLAUDE.md

This file provides guidance to Claude Code (claude.ai/code) when working with code in this repository.

## Project

An idle/clicker game for an ESP32 with a 240x320 ILI9341 SPI display, four push buttons, a passive buzzer, a DHT11 and an LDR. PlatformIO + Arduino framework, C++. Two halves feed each other: a Cookie-Clicker-style gold economy (mining clicks, buildings, one-time upgrades) and an idle combat loop (zones, enemies, item drops, equipment).

## Commands

`pio` is not on `PATH`; it lives at `~/.platformio/penv/bin/pio`.

```bash
pio run                  # Build
pio run -t upload        # Build and flash
pio device monitor       # Serial monitor (115200)
pio run -t upload -t monitor
pio run -t clean
```

There are no tests (`test/` is empty); `pio test` would run them if added.

## Hardware configuration

- **All TFT_eSPI setup lives in `platformio.ini` `build_flags`**, not in a `User_Setup.h`. This is deliberate so the config survives `pio run -t clean` and library updates. Changing display wiring means editing those `-D` flags.
- **Pin assignments are `const int` at the top of `src/main.cpp`.** Two constraints are load-bearing: the DHT pin must be output-capable (so not GPIO 34-39), and the LDR must be on an ADC1 pin (34-39) because ADC2 is unusable while WiFi is active. Buttons are `INPUT_PULLUP`, so they also need pins that are neither flash (6-11) nor input-only, and avoiding strapping pins (0, 2, 5, 12, 15) keeps boot behaviour clean.
- **Five buttons:** menu (23) cycles forward through the screens, back (26) steps backward, confirm (27), up (33) and down (22) are the three in-screen actions.

## Architecture

### Main loop
`src/main.cpp` owns every object (display, buttons, managers, state, all screens) as globals and wires them together in `setup()`. `loop()` is strictly cooperative — **no blocking `delay()` anywhere in game code**. Every periodic component takes `now = millis()` as a parameter and self-throttles by comparing against its own `lastMs`.

`GameState::update()` and `CombatState::update()` both run every loop regardless of which screen is visible, so idle production and an active fight keep progressing in the background.

Elapsed-time math uses unsigned subtraction (`now - lastUpdate`), and deadlines use a signed difference (`(long)(now - deadline) >= 0`), so both survive `millis()` rollover. Deadlines are pushed forward from `now` rather than accumulated (`deadline = now + interval`), which avoids a burst of catch-up ticks after a slow frame.

**Global declaration order matters.** `GameState` holds an `Inventory &` (for item gold bonuses) and `CombatState` holds both, so they must be declared in dependency order: `Inventory` → `GameState` → `CombatState`. Globals in one translation unit initialize in declaration order.

### Screens
`ScreenManager` holds `Screen*` in the order they were added in `setup()`; the menu button cycles forward through them and the back button steps backward. The current order is:

```
Mining → Buildings → Upgrade → Awards → Inventory → Zones → Combat → (wraps to Mining)
```

**Screen order is the navigation order, and each screen's `Header` labels name its neighbours as hardcoded strings.** Inserting or reordering a screen means updating those labels in the affected constructors by hand.

Header text is width-constrained: the size-2 title and the size-1 nav labels share one 28px bar, so a 12-char title leaves no room for labels on either side. That is why the achievements screen is titled **"Awards"** rather than "Achievements" — six characters is what fits with both neighbours named.

A `Screen` is a list of `UI*` elements plus a background color. Subclasses:
- declare their elements as **members** (not heap-allocated), initialize them in the constructor's init list with their layout coordinates, then `addElement(&member)`;
- override `update(now)` to push fresh values into elements;
- override `onConfirmPress()` / `onUpPress()` / `onDownPress()` for input;
- optionally override `onBackPress()`, which returns a bool: **true consumes the back button**, false lets `ScreenManager::handleBackPress()` fall through to `previousScreen()`. `InventoryScreen` claims it while inspecting items to return to the slot list;
- override `onEnter(tft)` when arriving needs to reset screen-local state (and then call `Screen::onEnter(tft)`).

`ScreenManager::update()` calls the current screen's `update()` then `render()` each loop. Screens follow a `REFRESH_MS = 100` pattern: expensive text rebuilds are gated to ~10 Hz while animations advance every frame. Button handlers set `lastRefresh = 0` to force an immediate refresh past that gate.

Only the visible screen's `update()` runs, so a screen that reacts to background events (a drop landing, a kill) diffs a monotonic counter from the engine against its own last-seen value, and resyncs those counters in `onEnter` so it does not replay a backlog on arrival. `CombatScreen` does this for kill sounds and drop notifications.

**Global notifications go through the `Header`.** Every screen registers its header with `Screen::setHeader(&header)` next to `addElement`, and the main loop pushes the active notice to whatever screen is visible via `getCurrentScreen()->applyNotice(...)`. While a notice is set the header paints it over the whole bar instead of the title and labels. This is the one place that is the same shape on every screen, so a notification needs no reserved space anywhere else.

### Rendering: dirty-flag, no full redraws
`src/ui/UI.h` is the base class and the core of the render strategy. The screen is never cleared per frame; instead each element tracks a `dirty` flag and the bounds of its **last** draw (`drawnX/Y/W/H`). `redraw()` erases the old area only when the element became hidden, moved, or resized, then redraws. Consequences to respect when adding UI:

- **Every mutator must be a no-op when the value is unchanged** and call `markDirty()` only on a real change. Writing fields directly bypasses this and leaves stale pixels; marking dirty unconditionally costs a full repaint (for `Image::setPixels` with a 128x128 sprite, a whole SPI blit).
- `eraseColor` is assigned by `Screen::addElement()` from the screen's background — don't set it by hand.
- An element whose painted area differs from `x/y/w/h` (e.g. `Text`, which is positioned by a TFT datum) overrides `erase()`.
- **Overlapping elements cannot erase independently, and `addElement` order cannot rescue them.** Render order is registration order, so whichever element is registered later erases *after* the earlier one drew: putting A first fixes the A-appears/B-hides transition and breaks B-appears/A-hides. Two elements that swap places in the same strip will black out part of each other in one direction whatever the order. Either give each element its own space, or composite the whole region into a `TFT_eSprite` and push it in one go (`OreDisplay`).

`Screen::onEnter()` fills the background and calls `resetDrawn()` on everything, so the first draw after a screen switch skips erasing.

### Vertical lists
Four screens navigate a vertical list with two buttons, via two pieces:

- **`src/ui/ListRow.h`** — one row: optional icon (1-bit or RGB565), title (size 2), subtitle (size 1), and two right-aligned value fields, plus `NORMAL`/`SELECTED`/`DIMMED`/`OWNED` states. Like `CoinDisplay`, it composes everything with direct `fillRect`/`drawRect`/`drawBitmap`/`drawString` calls inside `draw()` rather than child `UI` objects.
- **`src/ui/ListView.h`** — selection and scroll arithmetic only, no drawing. `VISIBLE_ROWS = 9` is the default and the maximum; a screen that needs room for a panel passes a smaller count to the constructor (`BuildingScreen` uses 8 for its detail panel).

`ListRow` frames are white and pixel icons treat black as transparent. `setFrameColor()` and `setSelectedFill()` are opt-in styling (a dark frame, plus a highlight bar on the selected row) that `BuildingScreen` uses and the other list screens do not yet.

The screen keeps a **fixed** `ListRow rows[ListView::VISIBLE_ROWS]` registered once in the constructor and fills row `i` from item `getFirstVisible() + i`, hiding the leftovers. Elements are never added or removed as the list scrolls — the dirty-flag system cannot cope with that.

`InventoryScreen` shows the two-mode variant of this: one `ListView` reused for the slot list and then the item list of the slot being inspected, with row 0 as `< Back` because two buttons leave no other way out.

### Game state
Three objects own all progress, all constructed in `main.cpp` and passed **by reference** into the screens that need them:

- **`src/game/GameState.h`** — gold, buildings, upgrades, mining level. Gold is fixed-point in thousandths (`GOLD_SCALE = 1000`) so fractional production like 0.1/s is exact; `productionRemainder` carries the sub-unit leftover between frames. `getProductionPerSecond()` returns scaled units and is `uint64_t` because the late buildings pass 4.3e9. Use `src/game/Format.h` (`formatAmount`, `formatRate`, `formatPerSecond`) for display — raw numbers won't fit the 240px width. Both cap at **one decimal place**: amounts switch to a K/M/B/T/Q suffix past 9999, and rates round to tenths rather than truncating the thousandths that `GOLD_SCALE` stores.
- **`src/game/Inventory.h`** — one byte per item (`0` = not owned, `>= 1` = level) plus the equipped item index per slot. A duplicate drop raises the item's level instead of stacking. `getRevision()` is bumped on every change so screens and combat notice without polling each item.
- **`src/game/CombatState.h`** — the fight: current zone, enemy HP, player HP, cooldowns, kill counts, drop rolls.
- **`src/game/Achievements.h`** — which achievements are unlocked and the gold production they add together. It deliberately **holds no references**: that is what lets `GameState` read its bonus while `evaluate()` takes the other three states as arguments, instead of a dependency cycle. It is self-throttled, so the main loop can call `evaluate()` every iteration.

`src/game/Stats.h` holds the `Stats` struct and `resolvePlayerStats(game, inv)`, the single source of truth for the player's attack/defense/max HP (base values + equipment + upgrade bonuses). Everything that displays or uses stats goes through it.

**The two halves cross over in both directions:** `UpgradeTarget::ATTACK`/`DEFENSE`/`MAX_HP`/`UNLOCK_ZONE` upgrades feed combat, and an `ItemDef`'s `goldProductionBonusPercent`/`clickBonusPercent` feed the economy through `GameState::getProductionBonusPercent()`/`getClickBonusPercent()`. Achievements add to the same production total, so milestones on either side pay out as gold.

`GameState` keeps the lifetime counters that achievements need (`totalClicks`, `totalGoldEarned`), and every income path goes through the private `creditGold()` so the lifetime total cannot drift from the balance.

### Combat difficulty
Damage is `attack - defense` **with no floor**: an attack that cannot beat the defense does literally nothing. This is deliberate and load-bearing — a floor of 1 would let the auto-attack grind down any enemy given enough time, which would make gear optional.

The consequence is the progression rule the enemy tables are tuned to: **a zone's defense sits above the attack of the gear the player arrives with, but below twice it.** The auto-attack is therefore blocked while the manual strike (`STRIKE_MULTIPLIER`) still breaks through, so a new zone has to be fought actively and only becomes idle-farmable once items are equipped and levelled. Defense works the same way in reverse, so a zone you have outgrown stops hurting you at all and farms itself.

Reference numbers the tables assume, for a full set at item level 1: player attack 10 (starting sword only), then 13 / 29 / 77 / 221 for gear tiers 1-4; player defense 7, then 20 / 58 / 172 / 514.

Three actions multiply that attack before the subtraction, which is what gives each a distinct reach: the auto-attack (x1, every `AUTO_ATTACK_MS`), the strike (`STRIKE_MULTIPLIER`, Confirm), and the smite (`SMITE_MULTIPLIER` on a long cooldown, Up, unlocked by equipping an amulet). A higher multiplier beats a higher defense, so the gate is softened by gear and by which actions are available, never bypassed.

`ZoneScreen` applies the same rule ahead of time: each zone row is labelled `IDLE` / `STRIKE` / `SMITE` / `TOO HARD` by the weakest action that gets through its hardest enemy, and the panel shows each enemy's per-action damage, the damage taken, and the zone's drops grayed until found. It duplicates the `attack - defense` formula from `CombatState::damageTo`, so a change to damage math must change both. `IconStrip` and `ui/IconUtil.h` (`pushIcon`/`pushIconGray`) are the shared way to draw a 16x16 icon in color or grayed out.

Because a blocked attack just leaves the health bar still, the combat screen shows the effective damage on each attack bar rather than leaving the player to subtract. The player's bar is **hidden outright** when its damage is 0, the enemy's goes full and gray (`enemy cannot hurt you`), and the message line only speaks up for the hard case where `getBestAttackDamage()` is 0 too, meaning no action reaches. `strike()` and `smite()` refuse rather than burning their cooldown when they would deal nothing.

### Persistence
Nothing is saved yet — a reboot starts fresh. But the shape for it is in place and should be kept: `GameState`, `Inventory` and `CombatState` each expose a POD `Snapshot` with `save()`/`load()`, aggregated by `SaveBlob` in `src/game/SaveGame.h` (whose `saveGame`/`loadGame` are no-op stubs over a future `Preferences`/NVS implementation).

**Persistent state must stay POD: only primitives and indices into the `constexpr` config tables, never a `String` or a pointer.** Names, descriptions, icons and sprites live in the tables; state stores the index. Adding a field to a `Snapshot` changes the layout, so bump `SAVE_VERSION`.

### Content is table-driven
Five headers hold `static constexpr` tables with `*_COUNT` derived via `sizeof`, so adding content is adding a row:

- **`src/game/GameConfig.h`** — `ORE_TIERS`, `BUILDINGS` (Cookie Clicker's curve: each tier ~10x the cost and ~5.5x the output, `costGrowthPercent = 115`), `GOLD_UPGRADES`. An upgrade is hidden until its `reqBuilding`/`reqBuildingLevel`/`reqMiningLevel` gates are met (Cookie-Clicker-style gates at 1, 5, 25, 50), which is what makes the shop reveal itself gradually. For `UNLOCK_ZONE` upgrades, `bonusPercent` carries the **zone index** instead of a percentage. Every ore tier shares one sprite, and `OreDisplay` palette-swaps its three colours (`ORE_SPRITE_PALETTE` in `MiningScreen.cpp`) for the tier's `palette` (light, mid, dark), so a new tier needs three shades rather than a new image.
- **`src/game/ItemConfig.h`** — `EquipSlot` (9 slots, all unlocked from the start), `ITEMS`, and the `ItemId` enum that drop tables reference by name. **The enum order must match the table order.** `scaleItemStat()` applies `ITEM_LEVEL_GROWTH_PERCENT` per level above 1. Each item has a `tier` (0-3 within its slot) that picks its name color from `ITEM_TIER_COLORS` (common/uncommon/rare/epic).
- **`src/game/CombatConfig.h`** — combat pacing constants, `ZONES`, per-zone `EnemyDef` arrays and per-enemy `DropDef` tables (chance in basis points). Zone indices line up with the `bonusPercent` of the `UNLOCK_ZONE` upgrades, so a zone only needs a `requiresUnlockUpgrade` flag rather than an upgrade index. **Enemy `defense` is the difficulty dial** — see the balance rule below.
- **`src/game/AchievementConfig.h`** — `ACHIEVEMENTS`, each a threshold on a counter the game already tracks (`AchKind`) plus the gold production it grants. Adding one is a row; adding a *new kind* of condition means a new `AchKind`, one `case` in `Achievements::getProgress` and one in `iconFor` in `AchievementScreen.cpp`. Names cap at 13 chars and descriptions at 28 to fit a list row beside its progress percentage. `Achievements::getProgress()` returns the counter each one watches (`isConditionMet` is just `progress >= amount`), so a new `AchKind` is one `case` there.

Note the per-field comments: building and zone names cap at ~12 chars and item names at ~11 to fit a list row title at text size 2; upgrade names cap at 14 for the detail box, and descriptions wrap to two lines of 34.

`GameConfig.h` and `ItemConfig.h` also carry `BuildingId` and `ItemId` enums so other tables can point at a row by name. **Both enums must stay in the same order as the table they name.**

Each `BUILDINGS` row carries its own 16x16 `icon`, hand-drawn rather than taken from the icon library, which has no mining art. `BuildingScreen` hides a building as `???` until the one before it is owned. It still shows the price and can still be bought.

`UpgradeScreen` can only show 18 upgrades at once (a 6x3 grid of `UpgradeSlot` tiles, tinted and iconed per `UpgradeTarget` from its `CATEGORY_STYLES` table, which must stay in enum order), so it builds a `visibleIds[]` of unlocked-and-unbought upgrades each refresh and selects into **that** list, not into `GOLD_UPGRADES` directly.

### Assets
`src/assets/Assets.cpp` contains the image data as `PROGMEM` arrays (RGB565 `uint16_t` for color images, 1-bit packed `unsigned char` for monochrome icons), each declared `extern` in `Assets.h`. `Image` renders both, selected by constructor overload.

**Flash is the binding constraint on art.** `Assets.cpp` is already 211 KB of source, and `image_rato_pixels` (128x128 RGB565) alone is 32 KB. A 16x16 RGB565 icon costs 512 bytes; 32x32 costs 2 KB.

`src/assets/png/icons/` is a library of ~350 source PNGs that are **not** compiled and not referenced by the build (15 swords, 15 shields, 15 rings, 15 necklaces, 10 helmets, 24 potions, 5 armor/boots/gloves, and more). Using one means converting it to an array (e.g. via image2cpp), appending it to `Assets.cpp`, and adding the `extern` to `Assets.h`. Converted so far: one icon per item (material variants a/b/c/e for the four tiers; pants are hand-drawn and recolored from the armor of the same tier), per building, per upgrade category, and a few award icons. **Every enemy still shares the rat sprite** — `EnemyDef::sprite` exists so swapping in real art is a pointer change with no logic touched. Icons are generated by small Python scripts (PNG or ASCII art → byte-swapped RGB565, black = transparent); the asset words are byte-swapped because `pushImage` sends them raw.

### Managers
`src/managers/` holds header-only hardware wrappers: `Button` (interrupt-free polled debounce, exposing a one-shot `wasPressed()`), `SoundManager` (all buzzer notes in one place, using `pitches.h`; ESP32 `tone()` queues so sequences don't block), `ClimateManager` (DHT11, ≥2 s between reads) and `LuminosityManager` (LDR, exponentially smoothed, and the scale is **inverted** because this divider reads high in the dark).

## Performance notes

Screens rebuild display strings at 10 Hz, and `String` concatenation allocates. In refresh paths prefer `char buf[N]` + `snprintf` + `Text::setText(const char *)` / `ListRow` setters (which take `const String &` but compare before storing) over `String("x") + value`. For fields that are constant per list item, track which item each row currently shows and skip rebuilding them — `BuildingScreen::rowBuilding[]` is the example.
