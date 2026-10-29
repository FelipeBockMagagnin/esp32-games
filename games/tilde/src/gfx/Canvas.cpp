#include "Canvas.h"
#include <math.h>

static inline float clampf(float v, float lo, float hi)
{
    return v < lo ? lo : v > hi ? hi : v;
}

uint16_t Canvas::mix(uint16_t a, uint16_t b, uint8_t t)
{
    // TFT_eSPI's alphaBlend: red and blue together, then green
    uint32_t rb = a & 0xF81F;
    rb += ((b & 0xF81F) - rb) * (t >> 2) >> 6;
    uint32_t g = a & 0x07E0;
    g += ((b & 0x07E0) - g) * t >> 8;
    return (uint16_t)((rb & 0xF81F) | (g & 0x07E0));
}

void Canvas::begin(uint16_t *buffer, int16_t bandTop, int16_t bandRows)
{
    buf = buffer;
    top = bandTop;
    rows = bandRows;
    opacity = 255;
    clearClip();
}

void Canvas::blend(int16_t x, int16_t y, uint16_t color, uint16_t alpha)
{
    uint16_t &px = buf[(y - top) * W + x];
    px = alpha >= 255 ? color : mix(px, color, (uint8_t)alpha);
}

template <typename F>
void Canvas::shade(float x0, float y0, float x1, float y1, uint16_t color, uint8_t alpha, F distance)
{
    int16_t ix0 = (int16_t)fmaxf(floorf(x0), clipX0), ix1 = (int16_t)fminf(ceilf(x1), clipX1);
    int16_t iy0 = (int16_t)fmaxf(floorf(y0), fmaxf(top, clipY0));
    int16_t iy1 = (int16_t)fminf(ceilf(y1), fminf(top + rows, clipY1));
    uint16_t scale = (uint16_t)alpha * opacity / 255;
    if (scale == 0)
    {
        return;
    }
    for (int16_t y = iy0; y < iy1; y++)
    {
        float py = y + 0.5f;
        for (int16_t x = ix0; x < ix1; x++)
        {
            float d = distance(x + 0.5f, py);
            if (d >= 0.5f)
            {
                continue;
            }
            float cover = d <= -0.5f ? 1.0f : 0.5f - d;
            blend(x, y, color, (uint16_t)(cover * scale + 0.5f));
        }
    }
}

template <typename Extent, typename Distance>
void Canvas::spans(float x0, float y0, float x1, float y1, uint16_t color, uint8_t alpha, Extent extent,
                   Distance distance, uint16_t bottomColor, float gradH)
{
    int16_t r0 = (int16_t)fmaxf(floorf(y0 - 1), fmaxf(top, clipY0));
    int16_t r1 = (int16_t)fminf(ceilf(y1 + 1), fminf(top + rows, clipY1));
    uint16_t scale = (uint16_t)alpha * opacity / 255;
    if (scale == 0)
    {
        return;
    }
    for (int16_t row = r0; row < r1; row++)
    {
        float py = row + 0.5f;
        uint16_t c = gradH > 0 ? mix(color, bottomColor, (uint8_t)(clampf((py - y0) / gradH, 0, 1) * 255)) : color;
        // The edge's anti-aliased band reaches as far sideways as the shape changes over the rows
        // around this one; where a neighbour misses the shape, the whole row is edge
        float l[3], r[3];
        bool all = py > y0 + 1 && py < y1 - 1;
        for (int i = 0; i < 3 && all; i++)
        {
            all = extent(py + i - 1, l[i], r[i]);
        }
        if (!all)
        {
            shade(x0 - 1, row, x1 + 1, row + 1, c, alpha, distance);
            continue;
        }
        float innerL = fmaxf(l[0], fmaxf(l[1], l[2])) + 1, innerR = fminf(r[0], fminf(r[1], r[2])) - 1;
        float outerL = fminf(l[0], fminf(l[1], l[2])) - 1, outerR = fmaxf(r[0], fmaxf(r[1], r[2])) + 1;
        int16_t in0 = (int16_t)ceilf(innerL), in1 = (int16_t)floorf(innerR);
        int16_t e0 = (int16_t)floorf(outerL), e1 = (int16_t)ceilf(outerR);
        if (in1 <= in0)
        {
            in0 = in1 = e1; // too thin for an inside: all of it is edge
        }
        int16_t lo = in0 > clipX0 ? in0 : clipX0, hi = in1 < clipX1 ? in1 : clipX1;
        for (int16_t x = lo; x < hi; x++)
        {
            blend(x, row, c, scale);
        }
        for (int16_t pass = 0; pass < 2; pass++)
        {
            int16_t a = pass == 0 ? e0 : in1, b = pass == 0 ? in0 : e1;
            for (int16_t x = a > clipX0 ? a : clipX0; x < b && x < clipX1; x++)
            {
                float d = distance(x + 0.5f, py);
                if (d >= 0.5f)
                {
                    continue;
                }
                float cover = d <= -0.5f ? 1.0f : 0.5f - d;
                blend(x, row, c, (uint16_t)(cover * scale + 0.5f));
            }
        }
    }
}

void Canvas::clear(uint16_t color)
{
    for (int32_t i = 0; i < W * rows; i++)
    {
        buf[i] = color;
    }
}

void Canvas::fillRect(int16_t x, int16_t y, int16_t w, int16_t h, uint16_t color, uint8_t alpha)
{
    int16_t y0 = y < top ? top : y, y1 = y + h > top + rows ? top + rows : y + h;
    int16_t x0 = x < clipX0 ? clipX0 : x, x1 = x + w > clipX1 ? clipX1 : x + w;
    y0 = y0 < clipY0 ? clipY0 : y0;
    y1 = y1 > clipY1 ? clipY1 : y1;
    uint16_t a = (uint16_t)alpha * opacity / 255;
    for (int16_t row = y0; row < y1; row++)
    {
        for (int16_t col = x0; col < x1; col++)
        {
            blend(col, row, color, a);
        }
    }
}

void Canvas::gradient(int16_t x, int16_t y, int16_t w, int16_t h, uint16_t topColor, uint16_t bottomColor)
{
    int16_t y0 = y < top ? top : y, y1 = y + h > top + rows ? top + rows : y + h;
    for (int16_t row = y0; row < y1; row++)
    {
        uint16_t c = mix(topColor, bottomColor, (uint8_t)((row - y) * 255 / (h > 1 ? h - 1 : 1)));
        fillRect(x, row, w, 1, c);
    }
}

void Canvas::fillCircle(float cx, float cy, float r, uint16_t color, uint8_t alpha)
{
    if (r <= 0 || !reaches(cy - r - 1, cy + r + 1))
    {
        return;
    }
    spans(
        cx - r, cy - r, cx + r, cy + r, color, alpha,
        [&](float py, float &l, float &rr) {
            float dy = py - cy;
            if (dy * dy >= r * r)
            {
                return false;
            }
            float hw = sqrtf(r * r - dy * dy);
            l = cx - hw;
            rr = cx + hw;
            return true;
        },
        [&](float px, float py) {
            float dx = px - cx, dy = py - cy;
            return sqrtf(dx * dx + dy * dy) - r;
        });
}

void Canvas::fillEllipse(float cx, float cy, float rx, float ry, uint16_t color, uint8_t alpha)
{
    if (rx <= 0 || ry <= 0 || !reaches(cy - ry - 1, cy + ry + 1))
    {
        return;
    }
    float irx = 1.0f / rx, iry = 1.0f / ry;
    spans(
        cx - rx, cy - ry, cx + rx, cy + ry, color, alpha,
        [&](float py, float &l, float &r) {
            float u = (py - cy) * iry;
            if (u * u >= 1)
            {
                return false;
            }
            float hw = rx * sqrtf(1 - u * u);
            l = cx - hw;
            r = cx + hw;
            return true;
        },
        [&](float px, float py) {
            // The implicit function over its gradient: close to the true distance near the edge
            float ux = (px - cx) * irx, uy = (py - cy) * iry;
            float k = sqrtf(ux * ux + uy * uy);
            float gx = ux * irx, gy = uy * iry;
            float g = sqrtf(gx * gx + gy * gy);
            return g > 1e-6f ? (k - 1.0f) * k / g : -rx;
        });
}

static float roundRectDistance(float px, float py, float cx, float cy, float hw, float hh, float r)
{
    float qx = fabsf(px - cx) - (hw - r), qy = fabsf(py - cy) - (hh - r);
    float ox = qx > 0 ? qx : 0, oy = qy > 0 ? qy : 0;
    float inside = qx > qy ? qx : qy;
    return sqrtf(ox * ox + oy * oy) + (inside < 0 ? inside : 0) - r;
}

static bool roundRectExtent(float py, float cx, float cy, float hw, float hh, float r, float &l, float &rr)
{
    float dy = fabsf(py - cy) - (hh - r);
    if (dy >= r)
    {
        return false;
    }
    float half = dy <= 0 ? hw : hw - r + sqrtf(r * r - dy * dy);
    l = cx - half;
    rr = cx + half;
    return true;
}

void Canvas::fillRoundRect(float x, float y, float w, float h, float r, uint16_t color, uint8_t alpha)
{
    if (w <= 0 || h <= 0 || !reaches(y - 1, y + h + 1))
    {
        return;
    }
    float hw = w / 2, hh = h / 2, cx = x + hw, cy = y + hh;
    r = clampf(r, 0, hw < hh ? hw : hh);
    spans(
        x, y, x + w, y + h, color, alpha,
        [&](float py, float &l, float &rr) { return roundRectExtent(py, cx, cy, hw, hh, r, l, rr); },
        [&](float px, float py) { return roundRectDistance(px, py, cx, cy, hw, hh, r); });
}

void Canvas::fillRoundRectV(float x, float y, float w, float h, float r, uint16_t topColor, uint16_t bottomColor,
                            uint8_t alpha)
{
    if (w <= 0 || h <= 0 || !reaches(y - 1, y + h + 1))
    {
        return;
    }
    float hw = w / 2, hh = h / 2, cx = x + hw, cy = y + hh;
    r = clampf(r, 0, hw < hh ? hw : hh);
    spans(
        x, y, x + w, y + h, topColor, alpha,
        [&](float py, float &l, float &rr) { return roundRectExtent(py, cx, cy, hw, hh, r, l, rr); },
        [&](float px, float py) { return roundRectDistance(px, py, cx, cy, hw, hh, r); }, bottomColor, h);
}

void Canvas::strokeRoundRect(float x, float y, float w, float h, float r, float width, uint16_t color, uint8_t alpha)
{
    if (w <= 0 || h <= 0 || !reaches(y - 1, y + h + 1))
    {
        return;
    }
    float hw = w / 2, hh = h / 2, cx = x + hw, cy = y + hh, half = width / 2;
    r = clampf(r, 0, hw < hh ? hw : hh);
    shade(x - 1, y - 1, x + w + 1, y + h + 1, color, alpha, [&](float px, float py) {
        return fabsf(roundRectDistance(px, py, cx, cy, hw - half, hh - half, r > half ? r - half : 0)) - half;
    });
}

void Canvas::line(float x0, float y0, float x1, float y1, float width, uint16_t color, uint8_t alpha)
{
    float half = width / 2;
    float minY = (y0 < y1 ? y0 : y1) - half - 1, maxY = (y0 > y1 ? y0 : y1) + half + 1;
    if (!reaches(minY, maxY))
    {
        return;
    }
    float minX = (x0 < x1 ? x0 : x1) - half - 1, maxX = (x0 > x1 ? x0 : x1) + half + 1;
    float dx = x1 - x0, dy = y1 - y0, len2 = dx * dx + dy * dy, inv = len2 > 0 ? 1 / len2 : 0;
    shade(minX, minY, maxX, maxY, color, alpha, [&](float px, float py) {
        float t = clampf(((px - x0) * dx + (py - y0) * dy) * inv, 0, 1);
        float ex = px - (x0 + t * dx), ey = py - (y0 + t * dy);
        return sqrtf(ex * ex + ey * ey) - half;
    });
}

void Canvas::ring(float cx, float cy, float r, float width, uint16_t color, uint8_t alpha)
{
    float half = width / 2, out = r + half + 1;
    if (!reaches(cy - out, cy + out))
    {
        return;
    }
    shade(cx - out, cy - out, cx + out, cy + out, color, alpha, [&](float px, float py) {
        float dx = px - cx, dy = py - cy;
        return fabsf(sqrtf(dx * dx + dy * dy) - r) - half;
    });
}

void Canvas::arc(float cx, float cy, float r, float width, float start, float end, uint16_t color, uint8_t alpha)
{
    float half = width / 2, out = r + half + 1;
    if (!reaches(cy - out, cy + out))
    {
        return;
    }
    float sx = cx + cosf(start) * r, sy = cy + sinf(start) * r;
    float ex = cx + cosf(end) * r, ey = cy + sinf(end) * r;
    float span = end - start;
    shade(cx - out, cy - out, cx + out, cy + out, color, alpha, [&](float px, float py) {
        float dx = px - cx, dy = py - cy;
        float a = atan2f(dy, dx) - start;
        while (a < 0)
        {
            a += 2 * (float)M_PI;
        }
        while (a >= 2 * (float)M_PI)
        {
            a -= 2 * (float)M_PI;
        }
        if (a <= span)
        {
            return fabsf(sqrtf(dx * dx + dy * dy) - r) - half;
        }
        float d1 = sqrtf((px - sx) * (px - sx) + (py - sy) * (py - sy));
        float d2 = sqrtf((px - ex) * (px - ex) + (py - ey) * (py - ey));
        return (d1 < d2 ? d1 : d2) - half;
    });
}

void Canvas::fillTriangle(float x0, float y0, float x1, float y1, float x2, float y2, uint16_t color, uint8_t alpha)
{
    float minY = fminf(y0, fminf(y1, y2)) - 1, maxY = fmaxf(y0, fmaxf(y1, y2)) + 1;
    if (!reaches(minY, maxY))
    {
        return;
    }
    float minX = fminf(x0, fminf(x1, x2)) - 1, maxX = fmaxf(x0, fmaxf(x1, x2)) + 1;
    // Inigo Quilez's triangle distance
    float e0x = x1 - x0, e0y = y1 - y0, e1x = x2 - x1, e1y = y2 - y1, e2x = x0 - x2, e2y = y0 - y2;
    float s = e0x * e2y - e0y * e2x > 0 ? 1.0f : -1.0f;
    // Divisions are slow on the ESP32's FPU: the edges' inverse lengths once, not per pixel
    float i0 = 1 / (e0x * e0x + e0y * e0y), i1 = 1 / (e1x * e1x + e1y * e1y), i2 = 1 / (e2x * e2x + e2y * e2y);
    const float xs[3] = {x0, x1, x2}, ys[3] = {y0, y1, y2};
    spans(
        minX + 1, minY + 1, maxX - 1, maxY - 1, color, alpha,
        [&](float py, float &l, float &r) {
            l = 1e9f;
            r = -1e9f;
            // A sharp corner can poke out between two rows' centres: rows by a corner are all edge
            for (int i = 0; i < 3; i++)
            {
                if (fabsf(py - ys[i]) < 1)
                {
                    return false;
                }
            }
            for (int i = 0; i < 3; i++)
            {
                float ax = xs[i], ay = ys[i], bx = xs[(i + 1) % 3], by = ys[(i + 1) % 3];
                if ((py < ay && py < by) || (py > ay && py > by) || ay == by)
                {
                    continue;
                }
                float ix = ax + (py - ay) * (bx - ax) / (by - ay);
                l = fminf(l, ix);
                r = fmaxf(r, ix);
            }
            return r >= l;
        },
        [&](float px, float py) {
            float v0x = px - x0, v0y = py - y0, v1x = px - x1, v1y = py - y1, v2x = px - x2, v2y = py - y2;
            float t0 = clampf((v0x * e0x + v0y * e0y) * i0, 0, 1);
            float t1 = clampf((v1x * e1x + v1y * e1y) * i1, 0, 1);
            float t2 = clampf((v2x * e2x + v2y * e2y) * i2, 0, 1);
            float q0x = v0x - e0x * t0, q0y = v0y - e0y * t0;
            float q1x = v1x - e1x * t1, q1y = v1y - e1y * t1;
            float q2x = v2x - e2x * t2, q2y = v2y - e2y * t2;
            float d = fminf(q0x * q0x + q0y * q0y, fminf(q1x * q1x + q1y * q1y, q2x * q2x + q2y * q2y));
            float c0 = s * (v0x * e0y - v0y * e0x), c1 = s * (v1x * e1y - v1y * e1x), c2 = s * (v2x * e2y - v2y * e2x);
            float side = fminf(c0, fminf(c1, c2));
            return -sqrtf(d) * (side > 0 ? 1.0f : -1.0f);
        });
}

int16_t Canvas::textWidth(const char *text, const Font &font)
{
    int16_t w = 0;
    for (const char *p = text; *p != '\0'; p++)
    {
        uint8_t c = (uint8_t)*p;
        if (c >= 32 && c <= 126)
        {
            w += pgm_read_byte(&font.glyphs[c - 32].advance);
        }
    }
    return w;
}

int16_t Canvas::text(float x, float y, const char *text, const Font &font, uint16_t color, Align align, uint8_t alpha)
{
    int16_t width = textWidth(text, font);
    int16_t pen = (int16_t)lroundf(align == CENTER ? x - width / 2.0f : align == RIGHT ? x - width : x);
    int16_t lineTop = (int16_t)lroundf(y);
    if (!reaches(lineTop, lineTop + font.height))
    {
        return width;
    }
    uint16_t scale = (uint16_t)alpha * opacity / 255;
    for (const char *p = text; *p != '\0'; p++)
    {
        uint8_t c = (uint8_t)*p;
        if (c < 32 || c > 126)
        {
            continue;
        }
        Glyph g;
        memcpy_P(&g, &font.glyphs[c - 32], sizeof(g));
        uint8_t stride = (g.w + 1) / 2;
        for (uint8_t row = 0; row < g.h; row++)
        {
            int16_t py = lineTop + g.y + row;
            if (py < top || py >= top + rows || py < clipY0 || py >= clipY1)
            {
                continue;
            }
            for (uint8_t col = 0; col < g.w; col++)
            {
                int16_t px = pen + g.x + col;
                if (px < clipX0 || px >= clipX1)
                {
                    continue;
                }
                uint8_t byte = pgm_read_byte(&font.bits[g.offset + row * stride + col / 2]);
                uint8_t a4 = col & 1 ? byte & 0x0F : byte >> 4;
                if (a4 != 0)
                {
                    blend(px, py, color, (uint16_t)(a4 * 17) * scale / 255);
                }
            }
        }
        pen += g.advance;
    }
    return width;
}
