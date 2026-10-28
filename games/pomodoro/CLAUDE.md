# CLAUDE.md

This file provides guidance to Claude Code (claude.ai/code) when working with code in this repository.

## Project

A pomodoro timer and clock for the board of `esp32-idle-game`: an ESP32 with a 240x320 ILI9341 SPI display, five push buttons and a passive buzzer. PlatformIO + Arduino framework, C++. The UI framework (`src/ui/`, `Screen`, `ScreenManager`, themes) is ported from that game and works the same way.

## Code style

**Comment sparingly.** Write a comment only when the code cannot say it: a non-obvious constraint, a hardware quirk, or why something that looks wrong is deliberate. Do not narrate what the next line does, restate a name, label sections, or explain a change in a comment (that goes in the commit).

## Commands

`pio` is not on `PATH`; it lives at `~/.platformio/penv/bin/pio`.

```bash
pio run                       # Build
pio run -t upload -t monitor  # Flash and open the serial monitor (115200)
pio run -e dev -t upload -t monitor  # Dev build: serial commands end (finish the phase), screen <i>, shot
~/.platformio/penv/bin/python tools/screenshot.py out.png [--screen 1]  # Dev build on the board: the display as a PNG
```

**WiFi:** copy `include/secrets.example.h` to `include/secrets.h` (git-ignored) and fill in the network and the POSIX timezone. Without it the build falls back to the example, the WiFi stays off and the clock shows `--:--`; the timer works either way.

## Hardware

- **All TFT_eSPI setup lives in `platformio.ini` `build_flags`.** Fonts 1 (GLCD) and 7 (48px seven-segment digits, only `0-9 : - .`) are loaded.
- **Pins are `const int` at the top of `src/main.cpp`**, the same as the idle game's. D-pad: right (23) next screen, left (26) previous screen / back, confirm (27), up (19), down (22). Buzzer on 32.
- **Hold Left 1.5 s** opens Settings from any screen.
- **Backlight** (`TFT_BL`, 21) is PWM on LEDC channel 4, since `tone()` owns channel 0 and its timer. It dims after the Settings delay; the press that wakes it is swallowed, and so is the press that silences an alarm.

## Architecture

`src/main.cpp` owns every object as a global and wires them in `setup()`. **No blocking `delay()`**: everything periodic takes `now = millis()` and self-throttles. Elapsed time uses unsigned subtraction (`now - last`), deadlines a signed difference (`(long)(now - deadline) >= 0`), so both survive `millis()` rollover.

- **`src/app/`** holds the logic, with no TFT code. `FocusTimer` runs the phases (focus, break, a long break after every 4 focuses); a phase that runs out bumps `getPhaseEndEvents()`, which `checkPhaseEnd()` in `main.cpp` diffs to ring the alarm, count the focus and jump to the Focus screen. `Clock` joins the WiFi in the background and syncs over SNTP; `today()` is 0 until synced. `FocusStats` keeps a week of per-day counts, the streak and the totals.
- **Persistence** (`Storage.cpp`, NVS namespace `pomodoro`): settings under one key; stats in two slots written in turn with a sequence number and a checksum, so a power cut mid-write only loses that write. Stats are written only when a focus ends. Changing `FocusStats::Snapshot` means bumping `STATS_VERSION`.
- **Screens** cycle Clock → Focus → Stats (`main.cpp` and `screenshot.py` count on Focus being index 1); Settings and the reset `ConfirmScreen` are modals.

### Screens and rendering (as in the idle game)

A `Screen` subclass declares its `UI` elements as members, registers them once with `addElement(&member)` (`addElement(&e, Theme::CARD)` inside a filled card) and `setHeader(&header)`. `update(now)` pushes fresh values behind `if (!refreshDue(now)) return;`; input handlers call `forceRefresh()`.

Nothing is cleared per frame: each element tracks `dirty` and its last drawn bounds. **Every mutator is a no-op on an unchanged value.** Overlapping elements cannot erase each other cleanly; give each its own space. Font-7 text is drawn with an opaque background (`transparentBg` false) so a ticking time repaints without a flash; `Text` resets the font to 1 after drawing, since other elements assume it.

**Every UI colour comes from `src/ui/Theme.h`** as a `Color` slot, resolved in the active palette (`THEMES` in `Themes.h`) at draw time, so a stored colour must be a `Color`, never a `uint16_t`. A new slot needs a column in every `THEMES` row. Phase colours are in `screens/PhaseStyle.h`.

## Performance

Avoid `String` concatenation in refresh paths: use `char buf[N]` + `snprintf` + `setText(const char *)`.
