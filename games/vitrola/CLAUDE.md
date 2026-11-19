# CLAUDE.md

This file provides guidance to Claude Code (claude.ai/code) when working with code in this repository.

## Project

Vitrola, a record player for the passive buzzer, on the board of `games/idle`: an ESP32 with a 240x320 ILI9341 SPI display, five push buttons and a passive buzzer. PlatformIO + Arduino framework, C++. Its screen framework (`core/ScreenManager`, `Screen`, themes) came from `games/snake`. Four records: System Beeps by Shiru on two sides (CC-BY 3.0, credited in the Credits screen and `library.json`), and two of public-domain tunes arranged here for one square wave.

## Code style

**Comment sparingly.** Write a comment only when the code cannot say it: a non-obvious constraint, a hardware quirk, or why something that looks wrong is deliberate. Do not narrate what the next line does, restate a name, label sections, or explain a change in a comment (that goes in the commit).

## Commands

`pio` is not on `PATH`; it lives at `~/.platformio/penv/bin/pio`.

```bash
pio run                              # Build
pio run -t upload -t monitor         # Flash and open the serial monitor (115200)
pio run -e dev -t upload -t monitor  # Dev build: serial commands play <i>, shot, heap
~/.platformio/penv/bin/python tools/screenshot.py out.png  # Dev build on the board: the display as a PNG
tools/tracks.py                      # Rewrite src/music/Tracks.cpp and library.json from albums.json
tools/tracks.py --wav DIR [--only SLUG]  # Also each track as a WAV, to hear an arrangement on the PC
tools/test.sh                        # Every track decodes to its length, playlist modes, transport, save
tools/shots.sh [out_dir]             # The real screens drawn on the PC, one PNG per scripted scene
```

**`src/music/Tracks.cpp` is generated: never edit it.** Rerun `tools/tracks.py` after touching `albums.json`, `songs/` or `third_party/`, then `tools/test.sh`. `tools/make_mockups.py` takes the site's `screen.png` from the `02_system_beeps` scene.

## The music

- **Every track is one stream format, System Beeps' own**: a header byte (120, the frame rate), then `[frames, PIT period lo, hi]` events, a 255 holding one more frame, a 0 ending it. Frequency is 1193180 / period; period 0 is silence. `TrackReader` steps it exactly as Shiru's player does.
- **`third_party/system_beeps/`** holds Shiru's `.bin` files byte for byte, from the ESPboy port's `mus/`, and the album's own `sb.txt` with the licence. The port's player is GPL-3 and is not used; only the CC-BY data is.
- **`songs/*.txt`** are our transcriptions: `title`, `author`, `bpm` (quarters), optional `shelf` (a title for the shelf when the real one is over 16 letters), `transpose`, `lead` (`decay`, or `hold` for an organ), `attack`, `arp`, `arp_hold`, `bass_len`, then `melody:`, `bass:`, `chords:`, `drums:`. A note is `c#5/8.` (pitch, octave, 1/n of a whole, dots), the length sticks until changed, `r` rests, `~/4` ties, `[ ... ]N` repeats, `|` is only for the reader, `@140` changes tempo from there (in the melody; every voice follows it). Chords are `Am/2`, `D7`, `F#m`...; drums `k s h -`. A comment is `# ` with a space. `tools/tracks.py` warns when a voice's length differs from the melody's.
- **The arranger** (`arrange()` in `tools/tracks.py`) folds the voices into one square wave with System Beeps' tricks: drums and the melody's attack mute the rest, a held note decays into gaps the bass and a 120 Hz arpeggio of the chord fill, and a bass note that starts with a melody note sounds two frames before it.
- **Only public-domain tunes or openly licensed tracks**, each with its source and licence in `albums.json`. No film, game or pop melodies: they are owned, and the site distributes the firmware. Check a tune's composer died over 70 years ago and that it is not a later protected arrangement ("Parabéns a Você", for one, is protected).
- Titles go to the GLCD font, which has ASCII only: `tools/tracks.py` refuses anything else.

## Hardware

- **All TFT_eSPI setup lives in the repo root's `common.ini` (`[board]`)**; only the GLCD font.
- **Pins are `const int` at the top of `src/main.cpp`**: right 23, left 26, confirm 27, up 19, down 22, buzzer 32.
- **`Player` drives the buzzer itself** on LEDC channel 0, from an `esp_timer` at 120 Hz (8333 us), changing the frequency only when the period changes and writing `volumeDuty(loadVolume())` after it, as `lib/dotcore`'s `Volume.h` asks. `playTone()` would fight it for the channel, so the app makes no menu sounds. LEDC at 10 bits cannot go below about 80 Hz, so lower notes go up an octave (the tool does the same for our arrangements).
- **Backlight** (`TFT_BL`, 21) is PWM on LEDC channel 4 (`Backlight` in `lib/dotcore`). It dims after the Settings delay; the music keeps playing.
- **The game menu:** when the site installed the launcher, Settings has a Game menu row (`returnToLauncher()`), and holding Confirm while powering on opens it.

## Architecture

**No blocking `delay()`.** Elapsed time uses unsigned subtraction (`now - last`), deadlines a signed difference.

- **`src/app/`** is plain C++. `Playlist` knows the current track and the mode (Disco: the side to its end; Repetir; Faixa; Aleatorio) and which track comes next by hand or once one ends. `Transport` puts the buttons' verbs over `Player` and `Playlist`, starts the next track when one ends (`update()` every loop, whatever screen is up), and says where the needle sits across the side. `Save` (track and mode) goes through `SlotStore` under NVS namespace `vitrola`; adding a field means bumping `SAVE_VERSION`. Theme and dimming are `lib/dotcore`'s `Settings`; its `sound` field is unused.
- **`Player`** builds on a PC too (no timer, no sound): `tools/shots.cpp` calls `tick()` on the screens' clock.
- **Screens:** `PlayerScreen` is the only one in the rotation. `LibraryScreen` (the shelf, then a record's tracks, with Ajustes at the bottom), `SettingsScreen` and `CreditsScreen` are modals that swap with `replaceModal()`. On the player OK plays and pauses, Left and Right change track (Left past 3 s restarts it), Up changes the mode, Down opens the shelf. Up and Down repeat while held, for the lists.
- **`Turntable`** draws the plinth, record and arm at about 30 frames a second, pixel by pixel into a 26-row band buffer pushed with `pushImage()` (memory order, so colours are stored byte-swapped). Each pixel's radius comes from a square-root table: grooves, the lighter gaps between tracks and the fixed sheen depend only on it and the pixel's place, so only the label (a 61x61 picture drawn once per record) is rotated. The arm is placed by solving where a circle of its length around the pivot meets the groove the needle should be in; the needle crosses the side as the record plays, track after track. Everything outside the record is wood from `woodAt()`, the same pattern `drawPlinth()` paints, so a band never shows a seam.
- **`LcdPanel`** draws into a sprite and pushes it whole when something changed; titles over 18 letters scroll. **`VuMeter`** repaints only the segments that changed, from the levels `Player` keeps per pitch band.
- **Every UI colour comes from `src/ui/Theme.h`** as a `Color` slot, resolved in the active palette (`THEMES` in `Themes.h`): the slots `lib/dotui` needs, then the plinth, the display and the meter. A new slot needs a column in every `THEMES` row.
