// The panel as a framebuffer, drawing what the screens use of TFT_eSPI with its own GLCD and
// seven-segment fonts, so tools/shots.cpp can save the real screens as PNGs. The fonts are
// read from the library PlatformIO fetched (tools/shots.sh builds the firmware first).
#ifndef STUB_TFT_ESPI_H
#define STUB_TFT_ESPI_H

#include <Arduino.h>
#include <algorithm>
#include <vector>

#ifndef TFT_WIDTH
#define TFT_WIDTH 240
#define TFT_HEIGHT 320
#endif

#define TL_DATUM 0
#define TC_DATUM 1
#define TR_DATUM 2
#define ML_DATUM 3
#define MC_DATUM 4
#define MR_DATUM 5
#define BL_DATUM 6
#define BC_DATUM 7
#define BR_DATUM 8
#define L_BASELINE 9
#define C_BASELINE 10
#define R_BASELINE 11

#define TFT_BLACK 0x0000
#define TFT_WHITE 0xFFFF

namespace stubfont
{
#include <Fonts/glcdfont.c>
#include <Fonts/Font7srle.h>
} // namespace stubfont

class TFT_eSPI
{
public:
    std::vector<uint16_t> pixels;
    int32_t winX = 0, winY = 0, winW = 0, winH = 0, winAt = 0;

    TFT_eSPI(int16_t w = TFT_WIDTH, int16_t h = TFT_HEIGHT) { resize(w, h); }
    virtual ~TFT_eSPI() = default;

    int16_t width() const { return w; }
    int16_t height() const { return h; }
    uint16_t at(int16_t x, int16_t y) const { return pixels[y * w + x]; }

    void drawPixel(int32_t x, int32_t y, uint16_t c)
    {
        if (x >= clipX0 && x < clipX1 && y >= clipY0 && y < clipY1)
        {
            pixels[y * w + x] = c;
        }
    }
    void fillRect(int32_t x, int32_t y, int32_t rw, int32_t rh, uint16_t c)
    {
        for (int32_t j = y; j < y + rh; j++)
        {
            for (int32_t i = x; i < x + rw; i++)
            {
                drawPixel(i, j, c);
            }
        }
    }
    void fillScreen(uint16_t c) { fillRect(0, 0, w, h, c); }
    void drawFastHLine(int32_t x, int32_t y, int32_t len, uint16_t c) { fillRect(x, y, len, 1, c); }
    void drawFastVLine(int32_t x, int32_t y, int32_t len, uint16_t c) { fillRect(x, y, 1, len, c); }
    void drawRect(int32_t x, int32_t y, int32_t rw, int32_t rh, uint16_t c)
    {
        drawFastHLine(x, y, rw, c);
        drawFastHLine(x, y + rh - 1, rw, c);
        drawFastVLine(x, y, rh, c);
        drawFastVLine(x + rw - 1, y, rh, c);
    }
    void drawLine(int32_t x0, int32_t y0, int32_t x1, int32_t y1, uint16_t c)
    {
        int32_t dx = abs(x1 - x0), sx = x0 < x1 ? 1 : -1, dy = -abs(y1 - y0), sy = y0 < y1 ? 1 : -1;
        int32_t err = dx + dy;
        while (true)
        {
            drawPixel(x0, y0, c);
            if (x0 == x1 && y0 == y1)
            {
                break;
            }
            int32_t e2 = 2 * err;
            if (e2 >= dy)
            {
                err += dy;
                x0 += sx;
            }
            if (e2 <= dx)
            {
                err += dx;
                y0 += sy;
            }
        }
    }
    void fillCircle(int32_t cx, int32_t cy, int32_t r, uint16_t c) { fillEllipse(cx, cy, r, r, c); }
    void drawCircle(int32_t cx, int32_t cy, int32_t r, uint16_t c) { drawEllipse(cx, cy, r, r, c); }
    void fillEllipse(int32_t cx, int32_t cy, int32_t rx, int32_t ry, uint16_t c)
    {
        for (int32_t j = -ry; j <= ry; j++)
        {
            for (int32_t i = -rx; i <= rx; i++)
            {
                if (inside(i, j, rx, ry))
                {
                    drawPixel(cx + i, cy + j, c);
                }
            }
        }
    }
    void drawEllipse(int32_t cx, int32_t cy, int32_t rx, int32_t ry, uint16_t c)
    {
        for (int32_t j = -ry; j <= ry; j++)
        {
            for (int32_t i = -rx; i <= rx; i++)
            {
                bool edge = !inside(i - 1, j, rx, ry) || !inside(i + 1, j, rx, ry) || !inside(i, j - 1, rx, ry) ||
                            !inside(i, j + 1, rx, ry);
                if (inside(i, j, rx, ry) && edge)
                {
                    drawPixel(cx + i, cy + j, c);
                }
            }
        }
    }
    void fillRoundRect(int32_t x, int32_t y, int32_t rw, int32_t rh, int32_t r, uint16_t c)
    {
        for (int32_t j = 0; j < rh; j++)
        {
            for (int32_t i = 0; i < rw; i++)
            {
                if (inRound(i, j, rw, rh, r))
                {
                    drawPixel(x + i, y + j, c);
                }
            }
        }
    }
    void drawRoundRect(int32_t x, int32_t y, int32_t rw, int32_t rh, int32_t r, uint16_t c)
    {
        for (int32_t j = 0; j < rh; j++)
        {
            for (int32_t i = 0; i < rw; i++)
            {
                bool edge = !inRound(i - 1, j, rw, rh, r) || !inRound(i + 1, j, rw, rh, r) ||
                            !inRound(i, j - 1, rw, rh, r) || !inRound(i, j + 1, rw, rh, r);
                if (inRound(i, j, rw, rh, r) && edge)
                {
                    drawPixel(x + i, y + j, c);
                }
            }
        }
    }
    void fillTriangle(int32_t x0, int32_t y0, int32_t x1, int32_t y1, int32_t x2, int32_t y2, uint16_t c)
    {
        int32_t minX = std::min({x0, x1, x2}), maxX = std::max({x0, x1, x2});
        int32_t minY = std::min({y0, y1, y2}), maxY = std::max({y0, y1, y2});
        for (int32_t y = minY; y <= maxY; y++)
        {
            for (int32_t x = minX; x <= maxX; x++)
            {
                int64_t a = cross(x0, y0, x1, y1, x, y), b = cross(x1, y1, x2, y2, x, y), d = cross(x2, y2, x0, y0, x, y);
                if ((a >= 0 && b >= 0 && d >= 0) || (a <= 0 && b <= 0 && d <= 0))
                {
                    drawPixel(x, y, c);
                }
            }
        }
    }
    void drawTriangle(int32_t x0, int32_t y0, int32_t x1, int32_t y1, int32_t x2, int32_t y2, uint16_t c)
    {
        drawLine(x0, y0, x1, y1, c);
        drawLine(x1, y1, x2, y2, c);
        drawLine(x2, y2, x0, y0, c);
    }
    void drawBitmap(int16_t x, int16_t y, const uint8_t *bitmap, int16_t bw, int16_t bh, uint16_t c)
    {
        int16_t stride = (bw + 7) / 8;
        for (int16_t j = 0; j < bh; j++)
        {
            for (int16_t i = 0; i < bw; i++)
            {
                if (bitmap[j * stride + i / 8] & (0x80 >> (i & 7)))
                {
                    drawPixel(x + i, y + j, c);
                }
            }
        }
    }
    // The board's assets are byte-swapped, since pushImage sends memory order
    void pushImage(int32_t x, int32_t y, int32_t iw, int32_t ih, const uint16_t *data)
    {
        for (int32_t j = 0; j < ih; j++)
        {
            for (int32_t i = 0; i < iw; i++)
            {
                uint16_t raw = data[j * iw + i];
                drawPixel(x + i, y + j, (uint16_t)(raw >> 8 | raw << 8));
            }
        }
    }
    void pushImage(int32_t x, int32_t y, int32_t iw, int32_t ih, const uint16_t *data, uint16_t transparent)
    {
        for (int32_t j = 0; j < ih; j++)
        {
            for (int32_t i = 0; i < iw; i++)
            {
                uint16_t raw = data[j * iw + i];
                if (raw != transparent)
                {
                    drawPixel(x + i, y + j, (uint16_t)(raw >> 8 | raw << 8));
                }
            }
        }
    }
    // A window filled row by row with pushPixels, byte-swapped like pushImage
    void startWrite() {}
    void endWrite() {}
    void setAddrWindow(int32_t x, int32_t y, int32_t ww, int32_t wh)
    {
        winX = x;
        winY = y;
        winW = ww;
        winH = wh;
        winAt = 0;
    }
    void pushPixels(const void *data, uint32_t count)
    {
        const uint16_t *words = (const uint16_t *)data;
        for (uint32_t i = 0; i < count; i++, winAt++)
        {
            if (winW <= 0)
            {
                return;
            }
            uint16_t raw = words[i];
            drawPixel(winX + winAt % winW, winY + winAt / winW, (uint16_t)(raw >> 8 | raw << 8));
        }
    }
    uint16_t alphaBlend(uint8_t alpha, uint16_t fg, uint16_t bg)
    {
        auto mix = [&](int shift, int bits) {
            int mask = (1 << bits) - 1;
            int f = fg >> shift & mask, b = bg >> shift & mask;
            return ((f * alpha + b * (255 - alpha)) / 255) << shift;
        };
        return (uint16_t)(mix(11, 5) | mix(5, 6) | mix(0, 5));
    }

    void setViewport(int32_t x, int32_t y, int32_t vw, int32_t vh, bool = true)
    {
        clipX0 = std::max<int32_t>(0, x);
        clipY0 = std::max<int32_t>(0, y);
        clipX1 = std::min<int32_t>(w, x + vw);
        clipY1 = std::min<int32_t>(h, y + vh);
    }
    void resetViewport() { setViewport(0, 0, w, h); }

    void setTextFont(uint8_t f) { font = f; }
    void setTextSize(uint8_t s) { size = s > 0 ? s : 1; }
    void setTextColor(uint16_t c) { fg = bg = c; }
    void setTextColor(uint16_t c, uint16_t b) { fg = c, bg = b; }
    void setTextDatum(uint8_t d) { datum = d; }
    void setTextPadding(uint16_t p) { padding = p; }
    int16_t fontHeight() const { return (font == 7 ? chr_hgt_f7s : 8) * size; }
    int16_t textWidth(const String &s) const { return textWidth(s.c_str()); }
    int16_t textWidth(const char *s) const
    {
        int16_t width = 0;
        for (; *s; s++)
        {
            width += charWidth(*s);
        }
        return width * size;
    }
    int16_t drawString(const String &s, int32_t x, int32_t y) { return drawString(s.c_str(), x, y); }
    int16_t drawString(const char *s, int32_t x, int32_t y)
    {
        int16_t tw = textWidth(s), th = fontHeight();
        int32_t ox = x, oy = y;
        int column = datum % 3;
        if (datum >= L_BASELINE)
        {
            column = datum - L_BASELINE;
        }
        ox -= column == 1 ? tw / 2 : (column == 2 ? tw : 0);
        if (datum >= ML_DATUM && datum <= MR_DATUM)
        {
            oy -= th / 2;
        }
        else if (datum >= BL_DATUM && datum <= BR_DATUM)
        {
            oy -= th;
        }
        if (fg != bg)
        {
            int16_t padW = std::max<int16_t>(tw, padding);
            int32_t px = x - (column == 1 ? padW / 2 : (column == 2 ? padW : 0));
            fillRect(px, oy, padW, th, bg);
        }
        for (; *s; s++)
        {
            drawChar(*s, ox, oy);
            ox += charWidth(*s) * size;
        }
        return tw;
    }

protected:
    int16_t w = 0, h = 0;

    void resize(int16_t nw, int16_t nh)
    {
        w = nw;
        h = nh;
        pixels.assign((size_t)w * h, 0);
        resetViewport();
    }

private:
    int32_t clipX0 = 0, clipY0 = 0, clipX1 = 0, clipY1 = 0;
    uint8_t font = 1, size = 1, datum = TL_DATUM;
    uint16_t fg = 0xFFFF, bg = 0xFFFF, padding = 0;

    static bool inside(int32_t i, int32_t j, int32_t rx, int32_t ry)
    {
        double a = rx + 0.5, b = ry + 0.5;
        return (i * i) / (a * a) + (j * j) / (b * b) <= 1.0;
    }
    static bool inRound(int32_t i, int32_t j, int32_t rw, int32_t rh, int32_t r)
    {
        if (i < 0 || j < 0 || i >= rw || j >= rh)
        {
            return false;
        }
        int32_t cx = i < r ? r : (i >= rw - r ? rw - r - 1 : i);
        int32_t cy = j < r ? r : (j >= rh - r ? rh - r - 1 : j);
        return (i - cx) * (i - cx) + (j - cy) * (j - cy) <= r * r;
    }
    static int64_t cross(int32_t ax, int32_t ay, int32_t bx, int32_t by, int32_t px, int32_t py)
    {
        return (int64_t)(bx - ax) * (py - ay) - (int64_t)(by - ay) * (px - ax);
    }
    int16_t charWidth(char c) const
    {
        if (font != 7)
        {
            return 6;
        }
        uint8_t i = (uint8_t)c;
        return i >= 32 && i < 128 ? stubfont::widtbl_f7s[i - 32] : 0;
    }
    void drawChar(char c, int32_t x, int32_t y)
    {
        uint8_t code = (uint8_t)c;
        if (font != 7)
        {
            for (int col = 0; col < 5; col++)
            {
                uint8_t bits = stubfont::font[code * 5 + col];
                for (int row = 0; row < 8; row++)
                {
                    if (bits & (1 << row))
                    {
                        fillRect(x + col * size, y + row * size, size, size, fg);
                    }
                }
            }
            return;
        }
        if (code < 32 || code >= 128)
        {
            return;
        }
        // Runs of pixels row by row: the top bit marks a lit run, the rest is its length - 1
        const uint8_t *data = stubfont::chrtbl_f7s[code - 32];
        int32_t cw = stubfont::widtbl_f7s[code - 32], total = cw * chr_hgt_f7s, pc = 0;
        while (pc < total)
        {
            uint8_t run = *data++;
            int32_t len = (run & 0x7F) + 1;
            if (run & 0x80)
            {
                for (int32_t k = 0; k < len; k++, pc++)
                {
                    fillRect(x + (pc % cw) * size, y + (pc / cw) * size, size, size, fg);
                }
            }
            else
            {
                pc += len;
            }
        }
    }
};

class TFT_eSprite : public TFT_eSPI
{
public:
    explicit TFT_eSprite(TFT_eSPI *parent) : TFT_eSPI(0, 0), parent(parent) {}

    void setColorDepth(int8_t) {}
    void *createSprite(int16_t sw, int16_t sh)
    {
        resize(sw, sh);
        return pixels.data();
    }
    void deleteSprite() { resize(0, 0); }
    void pushSprite(int32_t x, int32_t y)
    {
        for (int16_t j = 0; j < h; j++)
        {
            for (int16_t i = 0; i < w; i++)
            {
                parent->drawPixel(x + i, y + j, at(i, j));
            }
        }
    }

private:
    TFT_eSPI *parent;
};

#endif // STUB_TFT_ESPI_H
