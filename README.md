# dotcompany Games

A site that installs the dotcompany ESP32 games on a board over USB, from the browser
(Chrome or Edge, through Web Serial and [ESP Web Tools](https://esphome.github.io/esp-web-tools/)).
Served by GitHub Pages from `docs/`.

## Publish a new version

```bash
./publish.sh          # every game in games.json
./publish.sh pet      # just one
git add docs && git commit -m "publish firmware" && git push
```

`publish.sh` builds each game from its local folder (`path` in `games.json`) with
PlatformIO, refusing a repo with uncommitted changes so a published version is always a
commit. It copies the four images to `docs/firmware/<id>/` and writes their manifest and
`docs/games.js`.

The images keep their own offsets (bootloader 0x1000, partitions 0x8000, boot_app0 0xE000,
app 0x10000) instead of one merged image, whose 0xFF padding would cover NVS at 0x9000 and
erase every game's save. Every game must keep the default partition table for that to hold.

## Add a game

1. A row in `games.json`: `id`, `name`, `tagline`, `description`, `path`, `env`, `image`, `accent`.
2. A 240x320 screen in `docs/img/<id>.png` (`tools/make_mockups.py` draws the current two).
3. `./publish.sh <id>`.

## Preview

```bash
python3 -m http.server -d docs 8000   # http://localhost:8000, a secure context like HTTPS
```

## GitHub Pages

Settings → Pages → Deploy from a branch → `main`, folder `/docs`.
