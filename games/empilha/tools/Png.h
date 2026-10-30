// Writes RGB565 pixels as a PNG, for the PC tools
#ifndef TOOLS_PNG_H
#define TOOLS_PNG_H

#include <cstdint>
#include <cstdio>
#include <string>
#include <zlib.h>

inline void writePng(const std::string &path, const uint16_t *pixels, int w, int h)
{
    std::string raw;
    for (int y = 0; y < h; y++)
    {
        raw.push_back(0);
        for (int x = 0; x < w; x++)
        {
            uint16_t c = pixels[y * w + x];
            raw.push_back((char)((c >> 11) * 255 / 31));
            raw.push_back((char)((c >> 5 & 63) * 255 / 63));
            raw.push_back((char)((c & 31) * 255 / 31));
        }
    }
    uLongf size = compressBound(raw.size());
    std::string packed(size, '\0');
    compress2((Bytef *)&packed[0], &size, (const Bytef *)raw.data(), raw.size(), 9);
    packed.resize(size);
    FILE *f = fopen(path.c_str(), "wb");
    auto be32 = [&](uint32_t v) {
        uint8_t b[4] = {(uint8_t)(v >> 24), (uint8_t)(v >> 16), (uint8_t)(v >> 8), (uint8_t)v};
        fwrite(b, 1, 4, f);
    };
    auto chunk = [&](const char *kind, const std::string &data) {
        be32(data.size());
        std::string body = std::string(kind, 4) + data;
        fwrite(body.data(), 1, body.size(), f);
        be32(crc32(0, (const Bytef *)body.data(), body.size()));
    };
    fwrite("\x89PNG\r\n\x1a\n", 1, 8, f);
    std::string ihdr;
    for (uint32_t v : {(uint32_t)w, (uint32_t)h})
    {
        for (int s = 24; s >= 0; s -= 8)
        {
            ihdr.push_back((char)(v >> s));
        }
    }
    ihdr += std::string("\x08\x02\x00\x00\x00", 5);
    chunk("IHDR", ihdr);
    chunk("IDAT", packed);
    chunk("IEND", "");
    fclose(f);
}

#endif // TOOLS_PNG_H
