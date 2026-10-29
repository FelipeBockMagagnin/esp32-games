# dotcompany Games

Every game for the dotcompany board (ESP32, 240x320 ILI9341, five buttons, a buzzer), the menu
that switches between them, the board's PCB and case, and the site that installs them over USB
from the browser (Chrome or Edge, through Web Serial and
[ESP Web Tools](https://esphome.github.io/esp-web-tools/)), served by GitHub Pages from `docs/`.

```
games/<id>/       one PlatformIO project per game, plus its game.json and screen.png
games/categories.json   the site's shelves, in page order
launcher/         the menu that boots the chosen game when several share a board
lib/              the engine the games share: dotcore (buttons, backlight, saves, settings,
                  pitches.h, Launcher.h) and dotui (the UI elements)
common.ini        the board's PlatformIO config, read by every platformio.ini
hardware/         the carrier PCB and the printed case
docs/             the site
tools/            publish.py, make_mockups.py, make_brand.py
```

## Publish

```bash
tools/publish.py              # every game whose firmware changed since it was published
tools/publish.py idle pet     # just these (`launcher` for the menu)
tools/publish.py --all        # everything
tools/publish.py --list       # only the game list and the screens, building nothing
git add docs && git commit -m "publish firmware" && git push
```

A game's version is the last commit that touched its firmware (its `src/`, `include/`, `lib/`
and `platformio.ini`, plus the repo's `lib/` and `common.ini`), so editing a description or another game
rebuilds nothing. Each build starts from clean. The script refuses a game with uncommitted
changes, so a published version is always a commit, and one with git-ignored files under `src/`
or `include/` (the Pomodoro's `secrets.h` holds a WiFi password, which must never reach a public
firmware). It writes `docs/firmware/<id>/` (the four images, the manifest, the launcher's icon)
and `docs/games.js`, and drops what is left of a game whose folder is gone.

The images keep their own offsets (bootloader 0x1000, partitions 0x8000, boot_app0 0xE000,
app 0x10000) instead of one merged image, whose 0xFF padding would cover NVS at 0x9000 and
erase every game's save. Every game must keep the default partition table for that to hold.

## Add a game

1. Copy the closest game to `games/<id>/`. The folder name is the id the site and the launcher use.
2. Give it its own NVS namespace, and make its reset clear only that namespace.
3. Write `game.json`:
   - `name`, `tagline`, `category` (an `id` from `games/categories.json`), `description` and `accent`;
   - optionally `order` (its place on the page and in the menu; unset goes last), `env`
     (default `esp32dev`) and `icon` (`[x, y, factor]`: the 64x64 icon the launcher and the site
     show is a crop of the screen, `64 * factor` wide).
4. Add a 240x320 `screen.png` (`tools/make_mockups.py` draws the existing games').
5. Commit, then run `tools/publish.py`. The site picks the game up from its folder.

## The console

The console on the page is the printed case: its proportions, window and D-pad come from
`hardware/case/case.scad`, measured in `--mm` (5.5556px, so the display's 43.2mm come out at its
240 pixels). `tools/make_brand.py` draws the engraved wordmark, the back's character and the
favicons from `hardware/case/brand.scad`. Rerun it if the case art changes.

## Preview

```bash
python3 -m http.server -d docs 8000   # http://localhost:8000, a secure context like HTTPS
```

## GitHub Pages

Settings → Pages → Deploy from a branch → `main`, folder `/docs`.
