#!/usr/bin/env python3
"""Builds src/music/Tracks.cpp and library.json from the albums in albums.json: System Beeps'
tracks byte for byte, and our arrangements of public-domain tunes from songs/*.txt.

Every track ends up in System Beeps' own format, a PC speaker stream at 120 frames a second:
one header byte (the frame rate), then events of three bytes, [frames to hold, PIT period lo, hi],
where the period divides the 1193180 Hz PIT clock and 0 is silence; a byte 255 holds one more
frame unchanged and a 0 ends the track.

    tools/tracks.py               # Rewrite src/music/Tracks.cpp and library.json
    tools/tracks.py --wav DIR     # Also render every track as DIR/<slug>.wav, to listen on a PC
    tools/tracks.py --only SLUG   # With --wav, just that one
"""
import argparse
import json
import math
import re
import struct
import sys
import wave
from fractions import Fraction
from pathlib import Path

ROOT = Path(__file__).resolve().parent.parent
PIT = 1193180
FPS = 120
# Size 2 letters that fit a row of the shelf; the display scrolls a longer title
SHELF_CHARS = 16
# The buzzer barely sounds below this, and LEDC at 10 bits cannot go much lower
MIN_HZ = 80

NOTE_INDEX = {"c": 0, "d": 2, "e": 4, "f": 5, "g": 7, "a": 9, "b": 11}
CHORDS = {"": (0, 4, 7), "m": (0, 3, 7), "7": (0, 4, 7, 10), "m7": (0, 3, 7, 10), "maj7": (0, 4, 7, 11),
          "dim": (0, 3, 6), "dim7": (0, 3, 6, 9), "aug": (0, 4, 8), "sus4": (0, 5, 7), "5": (0, 7)}


def midi_hz(m):
    return 440.0 * 2 ** ((m - 69) / 12)


def period(hz):
    while hz < MIN_HZ:
        hz *= 2
    return max(1, round(PIT / hz))


# --- Parsing ---------------------------------------------------------------------------------

def tokens(text):
    """Expands [ ... ]N repeats; | is a bar line for the reader only"""
    out, stack = [], [[]]
    for tok in text.replace("|", " ").split():
        if tok == "[":
            stack.append([])
        elif re.fullmatch(r"\]\d*", tok):
            body = stack.pop()
            stack[-1].extend(body * int(tok[1:] or 2))
        else:
            stack[-1].append(tok)
    if len(stack) != 1:
        raise ValueError("unbalanced [ ]")
    return stack[0]


def length(spec, last):
    if not spec:
        return last
    m = re.fullmatch(r"(\d+)(\.*)", spec)
    if not m:
        raise ValueError(f"bad length /{spec}")
    d = Fraction(1, int(m.group(1)))
    total, add = d, d
    for _ in m.group(2):
        add /= 2
        total += add
    return total


def parse_voice(text, transpose):
    """Notes as (start, length, midi or None) in whole notes, and tempo changes as (start, bpm)"""
    notes, tempos, pos, last = [], [], Fraction(0), Fraction(1, 4)
    for tok in tokens(text):
        if tok.startswith("@"):
            tempos.append((pos, float(tok[1:])))
            continue
        if tok.startswith("~"):
            last = length(tok[2:] if tok.startswith("~/") else "", last)
            if not notes:
                raise ValueError("~ ties nothing")
            s, d, midi = notes[-1]
            notes[-1] = (s, d + last, midi)
            pos += last
            continue
        m = re.fullmatch(r"([a-g])(#|b)?(\d)(?:/(\S+))?|r(?:/(\S+))?", tok)
        if not m:
            raise ValueError(f"bad note {tok!r}")
        last = length(m.group(4) or m.group(5), last)
        if m.group(1):
            midi = 12 * (int(m.group(3)) + 1) + NOTE_INDEX[m.group(1)] + {"#": 1, "b": -1, None: 0}[m.group(2)]
            notes.append((pos, last, midi + transpose))
        pos += last
    return notes, tempos, pos


def parse_chords(text, transpose):
    chords, pos, last = [], Fraction(0), Fraction(1, 4)
    for tok in tokens(text):
        name, _, spec = tok.partition("/")
        last = length(spec, last)
        if name not in ("r", "-"):
            m = re.fullmatch(r"([A-G])(#|b)?(maj7|m7|dim7|dim|aug|sus4|m|7|5)?", name)
            if not m:
                raise ValueError(f"bad chord {tok!r}")
            root = NOTE_INDEX[m.group(1).lower()] + {"#": 1, "b": -1, None: 0}[m.group(2)] + transpose
            chords.append((pos, last, root % 12, CHORDS[m.group(3) or ""]))
        pos += last
    return chords, pos


def parse_drums(text):
    hits, pos, last = [], Fraction(0), Fraction(1, 4)
    for tok in tokens(text):
        name, _, spec = tok.partition("/")
        last = length(spec, last)
        if name not in ("-", "r"):
            if name not in ("k", "s", "h"):
                raise ValueError(f"bad drum {tok!r}")
            hits.append((pos, name))
        pos += last
    return hits, pos


def parse_song(path):
    meta, sections, current = {}, {}, None
    for raw in path.read_text().splitlines():
        # A comment is a # and a space, which no sharp (c#4) has
        line = re.sub(r"(^|\s)#(\s.*)?$", "", raw).rstrip()
        if not line.strip():
            continue
        m = re.fullmatch(r"(\w+):\s*(.*)", line.strip())
        if m and m.group(1) in ("melody", "bass", "chords", "drums"):
            current = m.group(1)
            sections[current] = sections.get(current, "") + " " + m.group(2)
        elif m and current is None:
            meta[m.group(1)] = m.group(2)
        elif current is not None:
            sections[current] += " " + line
        else:
            raise ValueError(f"{path.name}: stray line {raw!r}")
    return meta, sections


# --- Arranging -------------------------------------------------------------------------------

class TimeMap:
    """Whole notes to frames through the melody's tempo changes"""

    def __init__(self, bpm, changes):
        self.points = [(Fraction(0), float(bpm))] + sorted(changes)
        self.starts = [0.0]
        for (p0, b0), (p1, _) in zip(self.points, self.points[1:]):
            self.starts.append(self.starts[-1] + float(p1 - p0) * 4 * 60 * FPS / b0)

    def frame(self, pos):
        i = max(k for k, (p, _) in enumerate(self.points) if p <= pos)
        p, bpm = self.points[i]
        return round(self.starts[i] + float(pos - p) * 4 * 60 * FPS / bpm)


def arrange(meta, sections, name):
    """One square wave from a melody, bass, chords and drums, with the tricks System Beeps
    describes: the melody's attack and the drums mute everything else, a held note decays into
    gaps that the bass and a fast arpeggio of the chord fill, and a bass note that starts with a
    melody note sounds for two frames before it"""
    transpose = int(meta.get("transpose", 0))
    melody, tempos, end = parse_voice(sections["melody"], transpose)
    tm = TimeMap(meta.get("bpm", 120), tempos)
    total = tm.frame(end)
    lead = meta.get("lead", "decay")
    attack = int(meta.get("attack", 9))
    arp_rate = int(meta.get("arp", 1))
    beat = Fraction(1, int(meta.get("beat", 4)))

    def spans(notes):
        out = []
        for start, dur, midi in notes:
            f0, f1 = tm.frame(start), tm.frame(start + dur)
            if f1 > f0:
                out.append((f0, f1, midi))
        return out

    lead_at = [None] * total
    for f0, f1, midi in spans(melody):
        cut = f1 - 1 if f1 - f0 > 3 else f1
        for f in range(f0, min(cut, total)):
            lead_at[f] = (f0, midi)

    bass_at = [None] * total
    bass_len = int(meta.get("bass_len", 6))
    if "bass" in sections:
        bass, _, bend = parse_voice(sections["bass"], transpose)
        if bend != end:
            print(f"  {name}: bass is {float(bend)} wholes, melody {float(end)}", file=sys.stderr)
        for f0, f1, midi in spans(bass):
            for f in range(f0, min(f0 + min(bass_len, f1 - f0), total)):
                bass_at[f] = (f0, midi)

    chord_at = [None] * total
    if "chords" in sections:
        chords, cend = parse_chords(sections["chords"], transpose)
        if cend != end:
            print(f"  {name}: chords are {float(cend)} wholes, melody {float(end)}", file=sys.stderr)
        octave = int(meta.get("chord_octave", 4))
        root_bass = "bass" not in sections and meta.get("bass_from_chords", "yes") == "yes"
        for start, dur, root, shape in chords:
            tones = sorted(12 * (octave + 1) + (root + s) % 12 for s in shape)
            # The arpeggio sounds for most of each beat, so the beats stay audible
            p = start
            while p < start + dur:
                f0, f1 = tm.frame(p), tm.frame(min(p + beat, start + dur))
                hold = max(1, int((f1 - f0) * float(meta.get("arp_hold", 0.7))))
                for f in range(f0, min(f0 + hold, total)):
                    chord_at[f] = tones
                p += beat
            if root_bass:
                f0 = tm.frame(start)
                low = 12 * (int(meta.get("bass_octave", 3)) + 1) + root
                for f in range(f0, min(f0 + bass_len, tm.frame(start + dur), total)):
                    bass_at[f] = (f0, low)

    drum_at = [None] * total
    if "drums" in sections:
        hits, _ = parse_drums(sections["drums"])
        for pos, kind in hits:
            f = tm.frame(pos)
            shape = {"k": [180, 120, 80], "s": [1800, 900], "h": [5200]}[kind]
            for i, hz in enumerate(shape):
                if f + i < total:
                    drum_at[f + i] = hz

    out = []
    for f in range(total):
        hz = None
        lead_note, bass_note = lead_at[f], bass_at[f]
        if drum_at[f] is not None:
            hz = drum_at[f]
        elif lead_note is not None:
            t = f - lead_note[0]
            click = bass_note is not None and bass_note[0] == lead_note[0]
            if click and t < 2:
                hz = midi_hz(bass_note[1])
            elif t < attack + (2 if click else 0) or lead == "hold":
                hz = midi_hz(lead_note[1])
            else:
                k = t - attack
                on = max(0, 3 - k // 30)
                if k % 4 < on:
                    hz = midi_hz(lead_note[1])
        if hz is None:
            if bass_note is not None and (lead_note is None or f % 2 == 0):
                hz = midi_hz(bass_note[1])
            elif chord_at[f] is not None:
                tones = chord_at[f]
                hz = midi_hz(tones[(f // arp_rate) % len(tones)])
        out.append(period(hz) if hz else 0)
    return out


# --- Streams ---------------------------------------------------------------------------------

def encode(periods):
    data, i = bytearray([FPS]), 0
    while i < len(periods):
        j = i
        while j < len(periods) and periods[j] == periods[i] and j - i < 254:
            j += 1
        data += bytes([j - i]) + struct.pack("<H", periods[i])
        i = j
    data.append(0)
    return bytes(data)


def decode(data):
    """Each frame's period, as the player steps through them"""
    out, i, cur = [], 1, 0
    while i < len(data):
        w = data[i]
        i += 1
        if w == 0:
            break
        if w == 255:
            out.append(cur)
            continue
        cur = data[i] | data[i + 1] << 8
        i += 2
        out += [cur] * w
    return out


def render_wav(periods, path, rate=22050):
    with wave.open(str(path), "wb") as w:
        w.setnchannels(1)
        w.setsampwidth(1)
        w.setframerate(rate)
        buf, phase = bytearray(), 0.0
        per_frame = rate / FPS
        for n, p in enumerate(periods):
            count = int((n + 1) * per_frame) - int(n * per_frame)
            if p == 0:
                buf += bytes([128]) * count
                continue
            step = (PIT / p) / rate
            for _ in range(count):
                phase = (phase + step) % 1.0
                buf.append(168 if phase < 0.5 else 88)
        w.writeframes(bytes(buf))


# --- Output ----------------------------------------------------------------------------------

def c_string(s):
    if any(ord(ch) > 126 for ch in s):
        raise ValueError(f"{s!r}: the GLCD font has ASCII only")
    return '"' + s.replace("\\", "\\\\").replace('"', '\\"') + '"'


def hex_rgb565(h):
    v = int(h.lstrip("#"), 16)
    return "0x%04x" % (((v >> 19) & 0x1F) << 11 | ((v >> 10) & 0x3F) << 5 | ((v >> 3) & 0x1F))


def main():
    ap = argparse.ArgumentParser()
    ap.add_argument("--wav", type=Path)
    ap.add_argument("--only")
    args = ap.parse_args()

    albums = json.loads((ROOT / "albums.json").read_text())
    tracks, library = [], []
    for a, album in enumerate(albums):
        album["first"] = len(tracks)
        for t in album["tracks"]:
            if "bin" in t:
                data = (ROOT / t["bin"]).read_bytes()
                if data[-1] != 0:
                    data += b"\x00"
                info = t
            else:
                path = ROOT / "songs" / f"{t['song']}.txt"
                meta, sections = parse_song(path)
                data = encode(arrange(meta, sections, path.name))
                info = {**meta, **t}
            frames = decode(data)
            slug = t.get("song") or Path(t["bin"]).stem
            shelf = info.get("shelf", info["title"])
            if len(shelf) > SHELF_CHARS:
                raise ValueError(f"{slug}: {shelf!r} is wider than the shelf's {SHELF_CHARS} letters; give it a shelf name")
            tracks.append({"slug": slug, "album": a, "title": info["title"], "shelf": shelf, "author": info["author"],
                           "data": data, "frames": len(frames)})
            library.append({"title": info["title"], "author": info["author"], "genre": album["name"],
                            "license": info.get("license", album.get("license")),
                            "source": info.get("source", album.get("source"))})
            if args.wav and (not args.only or args.only == slug):
                args.wav.mkdir(parents=True, exist_ok=True)
                render_wav(frames, args.wav / f"{slug}.wav")
        album["count"] = len(tracks) - album["first"]
        album["frames"] = sum(t["frames"] for t in tracks[album["first"]:])

    lines = ["// Generated by tools/tracks.py from albums.json, songs/ and third_party/: edit those, not this", "",
             '#include "Tracks.h"', ""]
    for i, t in enumerate(tracks):
        body = ",".join(f"0x{b:02x}" for b in t["data"])
        lines.append(f"static const uint8_t T{i}[] = {{{body}}};")
    lines += ["", "const Album ALBUMS[] = {"]
    for a in albums:
        lines.append(f"    {{{c_string(a['name'])}, {c_string(a['artist'])}, {c_string(a['label'][0])}, "
                     f"{c_string(a['label'][1])}, {hex_rgb565(a['color'])}, {hex_rgb565(a['ink'])}, "
                     f"{a['first']}, {a['count']}, {a['frames']}}},")
    lines += ["};", f"const uint8_t ALBUM_COUNT = {len(albums)};", "", "const Track TRACKS[] = {"]
    for i, t in enumerate(tracks):
        lines.append(f"    {{{c_string(t['title'])}, {c_string(t['shelf'])}, {c_string(t['author'])}, {t['album']}, T{i}, "
                     f"{len(t['data'])}, {t['frames']}}},")
    lines += ["};", f"const uint16_t TRACK_COUNT = {len(tracks)};", ""]
    (ROOT / "src" / "music" / "Tracks.cpp").write_text("\n".join(lines))
    (ROOT / "library.json").write_text(
        "[\n" + ",\n".join("  " + json.dumps(e, ensure_ascii=False) for e in library) + "\n]\n")

    size = sum(len(t["data"]) for t in tracks)
    print(f"{len(tracks)} tracks in {len(albums)} albums, {size // 1024} KB, "
          f"{sum(t['frames'] for t in tracks) // FPS // 60} min")


if __name__ == "__main__":
    main()
