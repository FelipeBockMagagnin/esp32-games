#include "Turntable.h"
#include <math.h>
#include "Theme.h"

// pushImage() sends the buffer in memory order, and the panel wants the high byte first
static inline uint16_t swap(uint16_t c)
{
    return (uint16_t)(c << 8 | c >> 8);
}

static uint16_t mix(uint16_t a, uint16_t b, uint8_t t)
{
    uint16_t r = ((a >> 11) * (255 - t) + (b >> 11) * t) / 255;
    uint16_t g = (((a >> 5) & 63) * (255 - t) + ((b >> 5) & 63) * t) / 255;
    uint16_t bl = ((a & 31) * (255 - t) + (b & 31) * t) / 255;
    return (uint16_t)(r << 11 | g << 5 | bl);
}

static uint16_t grey(uint8_t v)
{
    return (uint16_t)((v >> 3) << 11 | (v >> 2) << 5 | (v >> 3));
}

static const float TAU = 6.2831853f;
static const float RPM_SPEED = 100.0f / 3.0f / 60.0f; // 33 1/3 turns a minute
static const float REST_ANGLE = 1.65f;
static const int16_t REST_X = 198;
static const int16_t REST_Y = 196;
static const int16_t LED_X = 46;
static const int16_t LED_Y = 240;

static TFT_eSPI *display = nullptr;

void Turntable::begin()
{
    for (uint32_t r2 = 0; r2 < sizeof(root); r2++)
    {
        root[r2] = (uint8_t)sqrtf((float)r2);
    }
    armAngle = armTarget = REST_ANGLE;
}

uint16_t Turntable::woodAt(int16_t sx, int16_t sy) const
{
    int16_t wave = (int16_t)((sx * 3 + (sy >> 2)) >> 5);
    bool grain = (sy + wave) % 7 == 0 || ((sy * 13 + sx / 9) % 29) == 0;
    return wood[grain ? 1 : 0];
}

void Turntable::buildColors()
{
    wood[0] = swap(Theme::WOOD);
    wood[1] = swap(Theme::WOOD_GRAIN);
    metal[0] = swap(Theme::METAL);
    metal[1] = swap(Theme::METAL_DARK);
    metal[2] = swap(grey(0x22));

    bool gap[R + 1] = {};
    if (album < ALBUM_COUNT)
    {
        const Album &a = ALBUMS[album];
        uint32_t sum = 0;
        for (uint8_t i = 0; i + 1 < a.count; i++)
        {
            sum += TRACKS[a.first + i].frames;
            float f = a.frames > 0 ? (float)sum / a.frames : 0;
            int16_t r = (int16_t)lroundf(GROOVE_OUT - f * (GROOVE_OUT - GROOVE_IN));
            if (r > GROOVE_IN && r < GROOVE_OUT)
            {
                gap[r] = true;
            }
        }
    }

    for (int16_t r = 0; r <= R + RIM; r++)
    {
        uint16_t base;
        if (r > R)
        {
            base = Theme::METAL_DARK;
        }
        else if (r >= R - 1)
        {
            base = grey(0x34);
        }
        else if (gap[r])
        {
            base = grey(0x30);
        }
        else if (r > GROOVE_OUT || r < GROOVE_IN)
        {
            base = grey(0x16);
        }
        else
        {
            base = grey(r % 2 ? 0x1a : 0x0e);
        }
        rings[0][r] = swap(base);
        rings[1][r] = swap(mix(base, grey(0x60), 80));
        rings[2][r] = swap(mix(base, grey(0xb0), 140));
    }
}

void Turntable::buildLabel()
{
    if (display == nullptr || album >= ALBUM_COUNT)
    {
        return;
    }
    const Album &a = ALBUMS[album];
    TFT_eSprite art(display);
    art.setColorDepth(16);
    if (art.createSprite(LABEL_D, LABEL_D) == nullptr)
    {
        return;
    }
    uint16_t paper = a.color, ink = a.ink;
    art.fillSprite(paper);
    art.drawCircle(LABEL_R, LABEL_R, LABEL_R - 3, mix(paper, ink, 140));
    art.fillTriangle(LABEL_R - 4, 2, LABEL_R + 4, 2, LABEL_R, 8, ink);
    art.setTextFont(1);
    art.setTextSize(1);
    art.setTextDatum(MC_DATUM);
    art.setTextColor(ink);
    art.drawString(a.label1, LABEL_R, LABEL_R - 11);
    art.drawString(a.label2, LABEL_R, LABEL_R + 11);
    art.setTextColor(mix(paper, ink, 150));
    art.drawString("33 1/3", LABEL_R, LABEL_R + 21);
    art.drawFastHLine(LABEL_R - 16, LABEL_R - 3, 9, mix(paper, ink, 150));
    art.drawFastHLine(LABEL_R + 8, LABEL_R - 3, 9, mix(paper, ink, 150));
    for (int16_t y = 0; y < LABEL_D; y++)
    {
        for (int16_t x = 0; x < LABEL_D; x++)
        {
            label[y * LABEL_D + x] = swap(art.readPixel(x, y));
        }
    }
    art.deleteSprite();
}

void Turntable::setAlbum(uint8_t index)
{
    if (index == album && theme == Theme::getActive())
    {
        return;
    }
    album = index;
    theme = Theme::getActive();
    buildColors();
    buildLabel();
    drawnTurn = -1;
}

float Turntable::angleFor(float progress) const
{
    if (progress < 0)
    {
        return REST_ANGLE;
    }
    if (progress > 1)
    {
        progress = 1;
    }
    float rn = GROOVE_OUT - progress * (GROOVE_OUT - GROOVE_IN);
    float px = PIVOT_X - CX, py = PIVOT_Y - CY;
    float d = sqrtf(px * px + py * py);
    float cosA = (d * d + rn * rn - (float)ARM_LEN * ARM_LEN) / (2 * d * rn);
    float a = atan2f(py, px) + acosf(cosA < -1 ? -1 : cosA > 1 ? 1 : cosA);
    float nx = CX + rn * cosf(a), ny = CY + rn * sinf(a);
    return atan2f(ny - PIVOT_Y, nx - PIVOT_X);
}

void Turntable::setNeedle(float progress)
{
    needle = progress;
    armTarget = angleFor(progress);
}

bool Turntable::update(unsigned long now)
{
    float dt = last == 0 ? 0 : (now - last) / 1000.0f;
    last = now;
    if (dt > 0.1f)
    {
        dt = 0.1f;
    }
    float want = spinning ? RPM_SPEED : 0;
    if (speed < want)
    {
        speed = fminf(want, speed + 0.9f * dt);
    }
    else if (speed > want)
    {
        speed = fmaxf(want, speed - 0.45f * dt);
    }
    turn += speed * dt;
    turn -= floorf(turn);

    float step = 1.1f * dt;
    float diff = armTarget - armAngle;
    armAngle += fabsf(diff) <= step ? diff : (diff > 0 ? step : -step);

    float turned = fabsf(turn - drawnTurn);
    turned = fminf(turned, 1 - turned);
    return drawnTurn < 0 || turned > 0.002f || fabsf(armAngle - drawnArm) > 0.002f;
}

void Turntable::drawPlinth(TFT_eSPI &tft)
{
    display = &tft;
    if (theme != Theme::getActive())
    {
        uint8_t keep = album;
        album = 255;
        setAlbum(keep);
    }
    tft.fillRoundRect(PLINTH_X, PLINTH_Y, PLINTH_W, PLINTH_H, 6, Theme::WOOD_EDGE);
    const int16_t w = PLINTH_W - 4;
    const int16_t chunk = W * BAND / w;
    for (int16_t top = PLINTH_Y + 2; top < PLINTH_Y + PLINTH_H - 2; top += chunk)
    {
        int16_t rows = top + chunk > PLINTH_Y + PLINTH_H - 2 ? PLINTH_Y + PLINTH_H - 2 - top : chunk;
        for (int16_t j = 0; j < rows; j++)
        {
            for (int16_t i = 0; i < w; i++)
            {
                band[j * w + i] = woodAt(PLINTH_X + 2 + i, top + j);
            }
        }
        tft.pushImage(PLINTH_X + 2, top, w, rows, band);
    }
    drawnTurn = -1;
}

void Turntable::put(int16_t sx, int16_t sy, int16_t top, uint16_t c)
{
    if (sx >= X && sx < X + W && sy >= top && sy < top + BAND && sy < Y + H)
    {
        band[(sy - top) * W + (sx - X)] = c;
    }
}

void Turntable::disc(float cx, float cy, float r, int16_t top, uint16_t c)
{
    int16_t y0 = (int16_t)floorf(cy - r), y1 = (int16_t)ceilf(cy + r);
    if (y1 < top || y0 >= top + BAND)
    {
        return;
    }
    float r2 = r * r;
    for (int16_t y = y0; y <= y1; y++)
    {
        float dy = y - cy;
        if (dy * dy > r2)
        {
            continue;
        }
        float half = sqrtf(r2 - dy * dy);
        for (int16_t x = (int16_t)ceilf(cx - half); x <= (int16_t)floorf(cx + half); x++)
        {
            put(x, y, top, c);
        }
    }
}

void Turntable::rod(float x0, float y0, float x1, float y1, float r, int16_t top, uint16_t c)
{
    if (fmaxf(y0, y1) + r < top || fminf(y0, y1) - r >= top + BAND)
    {
        return;
    }
    float len = sqrtf((x1 - x0) * (x1 - x0) + (y1 - y0) * (y1 - y0));
    int16_t steps = (int16_t)len + 1;
    for (int16_t s = 0; s <= steps; s++)
    {
        float t = (float)s / steps;
        disc(x0 + (x1 - x0) * t, y0 + (y1 - y0) * t, r, top, c);
    }
}

void Turntable::drawArm(int16_t top)
{
    float dx = cosf(armAngle), dy = sinf(armAngle);
    float nx = PIVOT_X + dx * ARM_LEN, ny = PIVOT_Y + dy * ARM_LEN;
    uint16_t shadow = swap(grey(0x08));

    disc(REST_X, REST_Y, 5, top, metal[1]);
    disc(REST_X, REST_Y, 3, top, metal[0]);

    rod(PIVOT_X + 3, PIVOT_Y + 5, nx + 3, ny + 5, 2, top, shadow);
    rod(PIVOT_X + dx * 6, PIVOT_Y + dy * 6, nx - dx * 10, ny - dy * 10, 2, top, metal[0]);
    rod(nx - dx * 13, ny - dy * 13, nx + dx * 2, ny + dy * 2, 3.5f, top, metal[1]);
    rod(nx - dx * 9, ny - dy * 9, nx - dx * 9 - dy * 8, ny - dy * 9 + dx * 8, 1, top, metal[0]);
    disc(nx, ny, 2.5f, top, metal[2]);

    disc(PIVOT_X - dx * 17, PIVOT_Y - dy * 17, 7, top, metal[1]);
    disc(PIVOT_X - dx * 17, PIVOT_Y - dy * 17, 5, top, metal[0]);
    disc(PIVOT_X, PIVOT_Y, 11, top, metal[1]);
    disc(PIVOT_X, PIVOT_Y, 8, top, metal[0]);
    disc(PIVOT_X, PIVOT_Y, 3, top, metal[1]);
}

void Turntable::render(TFT_eSPI &tft)
{
    display = &tft;
    float a = turn * TAU;
    int32_t c = (int32_t)lroundf(cosf(a) * 16384), s = (int32_t)lroundf(sinf(a) * 16384);
    const int32_t outer2 = (R + RIM) * (R + RIM);
    uint16_t led = swap(speed > 0.01f ? 0xF800 : grey(0x40));

    for (int16_t top = Y; top < Y + H; top += BAND)
    {
        int16_t rows = top + BAND > Y + H ? Y + H - top : BAND;
        for (int16_t j = 0; j < rows; j++)
        {
            int16_t sy = top + j;
            int32_t dy = sy - CY;
            uint16_t *row = band + j * W;
            for (int16_t i = 0; i < W; i++)
            {
                int16_t sx = X + i;
                int32_t dx = sx - CX;
                int32_t r2 = dx * dx + dy * dy;
                if (r2 > outer2)
                {
                    row[i] = woodAt(sx, sy);
                    continue;
                }
                int16_t r = root[r2];
                if (r < 3)
                {
                    row[i] = metal[r < 2 ? 0 : 1];
                }
                else if (r < LABEL_R)
                {
                    int32_t u = ((dx * c + dy * s) >> 14) + LABEL_R;
                    int32_t v = ((dy * c - dx * s) >> 14) + LABEL_R;
                    u = u < 0 ? 0 : u >= LABEL_D ? LABEL_D - 1 : u;
                    v = v < 0 ? 0 : v >= LABEL_D ? LABEL_D - 1 : v;
                    row[i] = label[v * LABEL_D + u];
                }
                else
                {
                    // A lamp up and to the left: the sheen runs along the diagonal through the centre
                    int32_t off = dx - dy;
                    off = off < 0 ? -off : off;
                    uint8_t level = off * 5 < r ? 2 : off * 2 < r ? 1 : 0;
                    row[i] = rings[level][r];
                }
            }
        }
        disc(LED_X - 18, LED_Y, 5, top, metal[1]);
        disc(LED_X - 18, LED_Y, 3, top, metal[0]);
        disc(LED_X, LED_Y, 3, top, led);
        drawArm(top);
        tft.pushImage(X, top, W, rows, band);
    }
    drawnTurn = turn;
    drawnArm = armAngle;
}
