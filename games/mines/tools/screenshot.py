#!/usr/bin/env python3
"""Saves what the board's display shows as a PNG, read back from the panel by the dev build's
`shot` serial command (pio run -e dev). Optionally switches screen first with `screen <index>`,
in addScreen() order (Title 0, Settings 1).

Needs pyserial, which PlatformIO's Python has:
    ~/.platformio/penv/bin/python tools/screenshot.py out.png [--screen 2] [--port /dev/ttyUSB0]

The serial monitor must be closed, since only one program can hold the port.
"""
import argparse
import struct
import sys
import time
import zlib

import serial


def write_png(path, width, height, rgb):
    def chunk(kind, data):
        body = kind + data
        return struct.pack(">I", len(data)) + body + struct.pack(">I", zlib.crc32(body) & 0xFFFFFFFF)

    stride = width * 3
    raw = b"".join(b"\x00" + rgb[y * stride:(y + 1) * stride] for y in range(height))
    with open(path, "wb") as f:
        f.write(b"\x89PNG\r\n\x1a\n")
        f.write(chunk(b"IHDR", struct.pack(">IIBBBBB", width, height, 8, 2, 0, 0, 0)))
        f.write(chunk(b"IDAT", zlib.compress(raw, 9)))
        f.write(chunk(b"IEND", b""))


def command(port, text):
    port.write(text.encode() + b"\n")
    port.flush()


def main():
    parser = argparse.ArgumentParser()
    parser.add_argument("out")
    parser.add_argument("--port", default="/dev/ttyUSB0")
    parser.add_argument("--screen", type=int)
    parser.add_argument("--settle", type=float, default=1.5, help="seconds to let a switched screen draw")
    args = parser.parse_args()

    # Opening the port resets most boards; let it boot past the splash
    with serial.Serial(args.port, 115200, timeout=30) as port:
        time.sleep(4)
        port.reset_input_buffer()
        if args.screen is not None:
            command(port, f"screen {args.screen}")
            time.sleep(args.settle)
            port.reset_input_buffer()
        command(port, "shot")
        while True:
            line = port.readline()
            if not line:
                sys.exit("no answer: is the dev build (pio run -e dev) on the board?")
            if line.startswith(b"SHOT "):
                width, height = (int(v) for v in line.split()[1:3])
                break
        size = width * height * 2
        data = port.read(size)
        if len(data) != size:
            sys.exit(f"short read: {len(data)} of {size} bytes")

    rgb = bytearray(width * height * 3)
    for i in range(width * height):
        c = data[2 * i] << 8 | data[2 * i + 1]
        rgb[3 * i] = (c >> 11) * 255 // 31
        rgb[3 * i + 1] = (c >> 5 & 63) * 255 // 63
        rgb[3 * i + 2] = (c & 31) * 255 // 31
    write_png(args.out, width, height, bytes(rgb))
    print(args.out)


if __name__ == "__main__":
    main()
