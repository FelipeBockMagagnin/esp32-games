#include "Rig.h"
#include <math.h>
#include "../gfx/Icons.h"
#include "../gfx/Theme.h"

const FormRig FORM_RIG[FORM_COUNT] = {
    {44, 54, BODY_E, 16, 9, 2, Feature::SPOTS, false, 0},      // Egg
    {54, 46, BODY_B, 20, 13, 2, Feature::ANTENNA, true, 1568}, // Blip
    {80, 62, BODY_A, 28, 15, 4, Feature::PLEAT, true, 1047},   // Mochi
    {72, 62, BODY_D, 26, 15, 6, Feature::EARS, true, 1319},    // Tuft
    {98, 70, BODY_B, 32, 17, 6, Feature::DRIPS, false, 880},   // Puddle
    {102, 60, BODY_C, 34, 14, 2, Feature::MOSS, true, 784},    // Pebble
    {90, 62, SHINE, 30, 16, 2, Feature::CLOUD, false, 1760},   // Nimbus
    {76, 68, BODY_E, 26, 15, 6, Feature::LEAVES, true, 1175},  // Sprout
};

// The body's geometry this frame, worked out once and shared by the parts
struct Frame
{
    float cx, cy, rx, ry, top, bottom, lean, k;
    uint16_t fill, outline, shade, shine, ink;
};

static uint16_t outlineOf(uint16_t fill)
{
    return Canvas::mix(fill, Theme::rgb(EYE), 70);
}

static void feature(Canvas &c, const FormRig &rig, const Pose &p, const Frame &f, bool back)
{
    float k = f.k;
    float tx = f.cx + f.lean, ty = f.top;
    uint16_t leaf = p.silhouette ? f.fill : Theme::rgb(LEAF);
    uint16_t leafEdge = outlineOf(leaf);
    switch (rig.feature)
    {
    case Feature::EARS:
        if (back)
        {
            for (int s = -1; s <= 1; s += 2)
            {
                float bx = tx + s * f.rx * 0.5f, tip = s * (f.rx * 0.78f) + p.sway * 3 * k;
                c.fillTriangle(bx - 13 * k * s, ty + 12 * k, bx + 5 * k * s, ty + 6 * k, tx + tip, ty - 14 * k, f.outline);
                c.fillTriangle(bx - 10 * k * s, ty + 12 * k, bx + 3 * k * s, ty + 7 * k, tx + tip * 0.97f, ty - 10 * k,
                               f.fill);
                if (!p.silhouette)
                {
                    c.fillTriangle(bx - 6 * k * s, ty + 9 * k, bx + 1 * k * s, ty + 7 * k, tx + tip * 0.93f, ty - 4 * k,
                                   Theme::rgb(BLUSH), 150);
                }
            }
        }
        break;
    case Feature::CLOUD:
        // Drawn with the body in drawPet, so the puffs and the body share one outline
        break;
    case Feature::ANTENNA:
    case Feature::LEAVES:
        if (!back)
        {
            float big = rig.feature == Feature::LEAVES ? 1.5f : 1.0f;
            float sx = tx + p.sway * 4 * k, sy = ty - 12 * k * big;
            c.line(tx, ty + 2, sx, sy, 3.2f * k, leafEdge);
            for (int s = -1; s <= 1; s += 2)
            {
                float lx = sx + s * 6 * k * big, ly = sy - 2 * k * big + (s > 0 ? p.sway : -p.sway) * k;
                c.fillEllipse(lx, ly, 7.5f * k * big, 5 * k * big, leafEdge);
                c.fillEllipse(lx, ly, 6 * k * big, 3.6f * k * big, leaf);
                if (!p.silhouette)
                {
                    c.fillEllipse(lx - s * k, ly - 1.2f * k, 2.4f * k * big, 1.2f * k * big, Theme::rgb(SHINE), 120);
                }
            }
        }
        break;
    case Feature::PLEAT:
        if (!back)
        {
            c.arc(tx, ty + 8 * k, 7 * k, 2.6f * k, 3.6f, 5.8f, f.outline, 200);
            c.fillCircle(tx, ty + 1.5f * k, 3 * k, f.outline);
            c.fillCircle(tx, ty + 1.5f * k, 2 * k, f.fill);
        }
        break;
    case Feature::MOSS:
        if (!back)
        {
            c.fillEllipse(tx - f.rx * 0.25f, ty + 4 * k, 10 * k, 4.5f * k, leaf);
            c.fillEllipse(tx + f.rx * 0.1f, ty + 2.5f * k, 7 * k, 3.5f * k, leaf);
            c.fillCircle(tx + f.rx * 0.32f, ty + 6 * k, 3 * k, leaf);
        }
        break;
    case Feature::DRIPS:
        if (!back)
        {
            for (int i = 0; i < 3; i++)
            {
                float u = -0.55f + i * 0.55f;
                float drip = (sinf(p.sway * 3.0f + i * 2.1f) * 0.5f + 0.5f) * 5 * k;
                float dx = f.cx + u * f.rx, dy = f.bottom - 3 * k + drip;
                c.fillEllipse(dx, dy, 4.5f * k, 5.5f * k + drip * 0.3f, f.outline);
                c.fillEllipse(dx, dy, 3.2f * k, 4.2f * k + drip * 0.3f, f.fill);
            }
        }
        break;
    case Feature::SPOTS:
        if (!back && !p.silhouette)
        {
            c.fillEllipse(f.cx - f.rx * 0.45f + f.lean * 0.4f, f.cy - f.ry * 0.2f, 5 * k, 6 * k, Theme::rgb(BLUSH), 170);
            c.fillEllipse(f.cx + f.rx * 0.4f + f.lean * 0.5f, f.cy - f.ry * 0.45f, 4 * k, 4.5f * k, Theme::rgb(GOLD),
                          170);
            c.fillEllipse(f.cx + f.rx * 0.3f + f.lean * 0.2f, f.cy + f.ry * 0.4f, 6 * k, 4 * k, Theme::rgb(WATER), 130);
        }
        break;
    }
}

static void eye(Canvas &c, const Pose &p, float ex, float ey, float e, bool left, uint16_t ink, uint16_t shine)
{
    float w = e * 0.13f;
    switch (p.eye)
    {
    case Eye::OPEN:
    case Eye::WIDE:
    case Eye::SAD:
    {
        float grow = p.eye == Eye::WIDE ? 1.2f : 1.0f;
        float px = ex + p.lookX * e * 0.14f, py = ey + p.lookY * e * 0.12f;
        c.fillEllipse(px, py, e * 0.36f * grow, e * 0.5f * grow, ink);
        c.fillCircle(px - e * 0.12f, py - e * 0.2f, e * 0.15f * grow, shine);
        c.fillCircle(px + e * 0.12f, py + e * 0.14f, e * 0.07f * grow, shine, 220);
        if (p.eye == Eye::SAD)
        {
            // Brows that rise toward the middle
            float in = left ? 1 : -1;
            c.line(ex - in * e * 0.4f, ey - e * 0.62f, ex + in * e * 0.15f, ey - e * 0.84f, w, ink);
        }
        break;
    }
    case Eye::HAPPY:
        c.arc(ex, ey + e * 0.2f, e * 0.3f, w * 1.3f, (float)M_PI + 0.35f, 2 * (float)M_PI - 0.35f, ink);
        break;
    case Eye::CLOSED:
        c.arc(ex, ey - e * 0.12f, e * 0.3f, w * 1.2f, 0.35f, (float)M_PI - 0.35f, ink);
        break;
    case Eye::BLINK:
        c.line(ex - e * 0.3f, ey, ex + e * 0.3f, ey, w * 1.2f, ink);
        break;
    case Eye::DIZZY:
        c.arc(ex, ey, e * 0.32f, w, 0, 5.2f, ink);
        c.arc(ex, ey, e * 0.14f, w, 3.0f, 7.6f, ink);
        break;
    case Eye::SQUEEZE:
    {
        float s = left ? 1 : -1;
        c.line(ex - s * e * 0.28f, ey - e * 0.26f, ex + s * e * 0.2f, ey, w * 1.2f, ink);
        c.line(ex + s * e * 0.2f, ey, ex - s * e * 0.28f, ey + e * 0.26f, w * 1.2f, ink);
        break;
    }
    case Eye::STAR:
        drawIcon(c, Icon::STAR, ex, ey, e * 1.1f, ink, ink);
        c.fillCircle(ex - e * 0.08f, ey - e * 0.06f, e * 0.08f, shine);
        break;
    case Eye::NONE:
        break;
    }
}

static void mouth(Canvas &c, const Pose &p, float mx, float my, float e, uint16_t ink, uint16_t inner)
{
    float w = e * 0.11f;
    switch (p.mouth)
    {
    case Mouth::SMILE:
        c.arc(mx, my - e * 0.12f, e * 0.2f, w, 0.5f, (float)M_PI - 0.5f, ink);
        break;
    case Mouth::W:
        c.arc(mx - e * 0.12f, my - e * 0.06f, e * 0.12f, w, 0.2f, (float)M_PI - 0.1f, ink);
        c.arc(mx + e * 0.12f, my - e * 0.06f, e * 0.12f, w, 0.1f, (float)M_PI - 0.2f, ink);
        break;
    case Mouth::OPEN:
        c.fillEllipse(mx, my + e * 0.04f, e * 0.15f, e * 0.18f, ink);
        c.fillEllipse(mx, my + e * 0.12f, e * 0.09f, e * 0.07f, inner);
        break;
    case Mouth::CHEW:
        c.fillEllipse(mx, my, e * 0.18f, e * 0.08f, ink);
        break;
    case Mouth::FROWN:
        c.arc(mx, my + e * 0.18f, e * 0.18f, w, (float)M_PI + 0.6f, 2 * (float)M_PI - 0.6f, ink);
        break;
    case Mouth::POUT:
        c.fillEllipse(mx, my, e * 0.09f, e * 0.08f, ink);
        break;
    case Mouth::FLAT:
        c.line(mx - e * 0.12f, my, mx + e * 0.12f, my, w, ink);
        break;
    case Mouth::NONE:
        break;
    }
}

static Frame frameOf(const FormRig &rig, const Pose &p)
{
    Frame f;
    f.k = p.scale;
    float puff = 1 + p.puff * 0.06f;
    f.rx = rig.w * 0.5f * p.scale * p.squashX * puff;
    f.ry = rig.h * 0.5f * p.scale * p.squashY;
    f.cx = p.x;
    f.cy = p.ground - p.lift - f.ry;
    f.top = f.cy - f.ry;
    f.bottom = f.cy + f.ry;
    f.lean = p.lean * p.scale;
    uint16_t fill = Theme::rgb(rig.body);
    if (p.pale)
    {
        fill = Canvas::mix(fill, Theme::rgb(INK_FAINT), 110);
    }
    if (p.flash > 0)
    {
        fill = Canvas::mix(fill, Theme::rgb(SHINE), (uint8_t)(p.flash * 255));
    }
    if (p.silhouette)
    {
        fill = Theme::rgb(INK_FAINT);
    }
    f.fill = fill;
    f.outline = p.silhouette ? Canvas::mix(fill, Theme::rgb(INK_SOFT), 60) : outlineOf(fill);
    f.shade = Canvas::mix(fill, f.outline, 60);
    f.shine = Theme::rgb(SHINE);
    f.ink = Theme::rgb(EYE);
    return f;
}

void petBounds(FormId form, const Pose &p, float &x0, float &y0, float &x1, float &y1)
{
    const FormRig &rig = FORM_RIG[form];
    Frame f = frameOf(rig, p);
    float extra = rig.feature == Feature::EARS      ? 16
                  : rig.feature == Feature::LEAVES  ? 26
                  : rig.feature == Feature::ANTENNA ? 18
                  : rig.feature == Feature::CLOUD   ? rig.h * 0.6f
                                                    : 4;
    x0 = f.cx - f.rx - fabsf(f.lean) - 3;
    x1 = f.cx + f.rx + fabsf(f.lean) + 3;
    y0 = f.top - extra * p.scale - 3;
    y1 = p.ground + 4;
}

void drawPet(Canvas &c, FormId form, const Pose &p)
{
    const FormRig &rig = FORM_RIG[form];
    Frame f = frameOf(rig, p);
    float bx0, by0, bx1, by1;
    petBounds(form, p, bx0, by0, bx1, by1);
    if (!c.reaches(by0, by1 + 6))
    {
        return;
    }
    float k = f.k;

    if (p.shadow)
    {
        float shrink = 1 / (1 + p.lift * 0.03f);
        c.fillEllipse(f.cx, p.ground + 1, f.rx * 0.85f * shrink, 4.5f * k * shrink, Theme::rgb(SHADOW), 150);
    }

    feature(c, rig, p, f, true);

    if (rig.feet)
    {
        for (int s = -1; s <= 1; s += 2)
        {
            float fx = f.cx + s * f.rx * 0.45f + (s * p.step) * 3 * k;
            float fy = f.bottom - 1 - (s * p.step > 0 ? 2 * k * fabsf(p.step) : 0);
            c.fillEllipse(fx, fy, 8 * k, 5.5f * k, f.outline);
            c.fillEllipse(fx, fy - 0.5f, 6.5f * k, 4 * k, f.shade);
        }
    }

    // The body leans by stacking: the outline and the fill, then a lighter top shifted with the lean.
    // A cloud's puffs go into both passes, so they merge with the body into one shape.
    static const float PUFFS[3][3] = {{-0.42f, 0.42f, 0.42f}, {0.06f, 0.62f, 0.48f}, {0.46f, 0.38f, 0.36f}};
    bool cloud = rig.feature == Feature::CLOUD;
    for (int pass = 0; pass < 2; pass++)
    {
        float grow = pass == 0 ? 1.8f : 0;
        uint16_t color = pass == 0 ? f.outline : f.fill;
        c.fillEllipse(f.cx + f.lean * 0.5f, f.cy, f.rx + grow, f.ry + grow, color);
        if (cloud)
        {
            for (auto &q : PUFFS)
            {
                c.fillCircle(f.cx + f.lean + f.rx * q[0], f.cy - f.ry * q[1], f.rx * q[2] + grow, color);
            }
        }
    }
    if (!p.silhouette)
    {
        c.fillEllipse(f.cx + f.lean * 0.4f, f.cy + f.ry * 0.45f, f.rx * 0.8f, f.ry * 0.42f, f.shade, 90);
        c.fillEllipse(f.cx + f.lean * 0.7f, f.cy - f.ry * 0.15f, f.rx * 0.82f, f.ry * 0.7f, f.fill);
        c.fillEllipse(f.cx - f.rx * 0.45f + f.lean, f.top + f.ry * 0.38f, f.rx * 0.16f, f.ry * 0.12f, f.shine, 170);
        c.fillCircle(f.cx - f.rx * 0.22f + f.lean, f.top + f.ry * 0.24f, f.rx * 0.05f + 1, f.shine, 140);
    }

    feature(c, rig, p, f, false);

    if (p.silhouette || p.eye == Eye::NONE)
    {
        return;
    }
    float e = rig.eyeSize * k;
    float faceX = f.cx + f.lean * 0.8f + p.lookX * 2.5f * k;
    float ey = f.cy - rig.eyeY * k * p.squashY + p.lookY * 2 * k;
    float gap = rig.eyeGap * 0.5f * k * p.squashX;
    float eyeL = faceX - gap, eyeR = faceX + gap;
    if (p.blush > 0)
    {
        uint8_t a = (uint8_t)(p.blush * 190);
        float br = e * (0.36f + p.puff * 0.2f);
        c.fillEllipse(eyeL - e * 0.55f, ey + e * 0.62f, br, br * 0.58f, Theme::rgb(BLUSH), a);
        c.fillEllipse(eyeR + e * 0.55f, ey + e * 0.62f, br, br * 0.58f, Theme::rgb(BLUSH), a);
    }
    eye(c, p, p.mirror ? eyeR : eyeL, ey, e, !p.mirror, f.ink, f.shine);
    eye(c, p, p.mirror ? eyeL : eyeR, ey, e, p.mirror, f.ink, f.shine);
    mouth(c, p, faceX, ey + e * 0.62f, e, f.ink, Theme::rgb(BLUSH));
}
