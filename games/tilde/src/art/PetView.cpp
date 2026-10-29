#include "PetView.h"
#include <math.h>
#include "../gfx/Icons.h"
#include "../gfx/Theme.h"

static const uint16_t ANIM_MS[(uint8_t)Anim::COUNT] = {0, 1900, 1100, 1500, 1700, 1400, 2800, 2200, 800, 900, 1200, 700};
static const unsigned long HATCH_CRACK_MS = 1300;
static const unsigned long HATCH_OPEN_MS = 1900;
static const unsigned long EVOLVE_POP_MS = 1000;
static const float GRAVITY = 900;
static const float TAU = 2 * (float)M_PI;

static float rnd(float lo, float hi)
{
    return lo + (hi - lo) * random(1001) / 1000.0f;
}

static float wave(unsigned long t, float periodMs, float phase = 0)
{
    return sinf(t * TAU / periodMs + phase);
}

float PetView::width(const Pet &pet) const
{
    return FORM_RIG[pet.getForm()].w;
}

float PetView::headY(const Pet &pet) const
{
    return Room::GROUND - lift - FORM_RIG[pet.getForm()].h;
}

void PetView::reset(unsigned long now, const Pet &pet)
{
    anim = Anim::NONE;
    act = Act::IDLE;
    actEnd = now + 1200;
    particles.clear();
    x = target = 120;
    stretch.reset();
    lift = liftV = 0;
    airborne = false;
    jumpAt = 0;
    nextBlink = now + 2500;
    nextGlance = now + 1500;
    nextAmbient = now + 1000;
    seenFluff = pet.getFluff();
    seenAsleep = pet.isAsleep();
    roomState.night = pet.isLightOn() ? 0 : 1;
    roomState.stars = pet.isAsleep();
    lastUpdate = now;
}

void PetView::hop(unsigned long now, float power, unsigned long crouchMs)
{
    // A crouch first, then up: the anticipation is what makes a jump read as springy
    kick(-3.5f);
    jumpAt = now + crouchMs;
    jumpPower = power;
}

void PetView::lookAt(float tx, float ty, unsigned long now, unsigned long ms)
{
    float dx = tx - x, dy = ty - (Room::GROUND - 30);
    lookToX = fmaxf(-1, fminf(1, dx / 60));
    lookToY = fmaxf(-1, fminf(1, dy / 60));
    lookUntil = now + ms;
}

void PetView::burst(unsigned long now, float cx, float cy, uint8_t count, bool confetti)
{
    static const uint8_t COLORS[] = {BLUSH, GOLD, WATER, LEAF, ACCENT};
    for (uint8_t i = 0; i < count; i++)
    {
        // Up and outward, clear of the pet's face
        float a = (float)M_PI + (i + 0.5f) * (float)M_PI / count + rnd(-0.15f, 0.15f), speed = rnd(110, 170);
        particles.spawn(now, confetti ? Bit::CONFETTI : Bit::SPARKLE, cx, cy, cosf(a) * speed, sinf(a) * speed - 30,
                        900, confetti ? 9 : 10, COLORS[i % sizeof(COLORS)], confetti ? 220 : 60);
    }
}

void PetView::play(Anim next, unsigned long now, const Pet &pet)
{
    anim = next;
    animStart = now;
    animEnd = now + ANIM_MS[(uint8_t)next];
    animStep = 0;
    float w = width(pet), top = headY(pet);
    switch (next)
    {
    case Anim::MEAL:
    case Anim::SNACK:
        // The bowl goes on whichever side has room, and the pet turns to it
        facingLeft = x + w * 0.5f + 34 > Room::WALK_RIGHT;
        act = Act::IDLE;
        break;
    case Anim::CUDDLE:
        kick(-5);
        for (int i = -1; i <= 1; i++)
        {
            particles.spawn(now + 0, Bit::HEART, x + i * 14, top - 2, i * 25.0f, -55, 1300, 14, BLUSH);
        }
        break;
    case Anim::CHEER:
        hop(now, 230);
        particles.spawn(now, Bit::SPARKLE, x - w * 0.5f, top, -20, -40, 900, 10, GOLD);
        particles.spawn(now, Bit::SPARKLE, x + w * 0.5f, top, 20, -40, 900, 10, GOLD);
        break;
    case Anim::REFUSE:
        kick(-2);
        break;
    case Anim::STARTLE:
        hop(now, 260, 0);
        kick(5);
        break;
    case Anim::WAKE:
        kick(4);
        break;
    case Anim::MEDS:
        for (int i = 0; i < 4; i++)
        {
            particles.spawn(now + i * 120, Bit::CROSS, x + rnd(-w * 0.6f, w * 0.6f), top + rnd(0, 20), 0, -45, 1100, 9,
                            WATER);
        }
        break;
    case Anim::EVOLVE:
        kick(-4);
        break;
    default:
        break;
    }
}

void PetView::choose(unsigned long now, const Pet &pet)
{
    // Weights for what to do next, by mood
    Mood mood = pet.mood();
    struct Option
    {
        Act act;
        int weight;
    };
    Option options[] = {
        {Act::IDLE, 3},
        {Act::WALK, 4},
        {Act::SIT, 2},
        {Act::LOOK, 2},
        {Act::YAWN, mood == Mood::SLEEPY ? 5 : 1},
        {Act::DANCE, mood == Mood::HAPPY ? 4 : 0},
        {Act::BOUNCE, mood == Mood::HAPPY ? 3 : mood == Mood::CONTENT ? 1 : 0},
        {Act::SNIFF, mood == Mood::CONTENT || mood == Mood::HAPPY ? 2 : 0},
        {Act::CHASE, mood == Mood::CONTENT || mood == Mood::HAPPY ? 2 : 0},
        {Act::SIGH, mood == Mood::SAD || mood == Mood::MEH ? 3 : 0},
        {Act::DOZE, mood == Mood::SLEEPY ? 4 : 0},
    };
    if (mood == Mood::SICK || mood == Mood::SAD)
    {
        options[1].weight = 1;
    }
    uint16_t total = 0;
    for (auto &o : options)
    {
        total += o.weight;
    }
    int16_t roll = random(total);
    for (auto &o : options)
    {
        roll -= o.weight;
        if (roll < 0)
        {
            act = o.act;
            break;
        }
    }
    actStart = now;
    float w = width(pet);
    float lo = Room::WALK_LEFT + w * 0.5f, hi = Room::WALK_RIGHT - w * 0.5f;
    switch (act)
    {
    case Act::WALK:
        target = lo < hi ? rnd(lo, hi) : (lo + hi) / 2;
        actEnd = now + 6000;
        break;
    case Act::SNIFF:
        target = hi;
        actEnd = now + 6500;
        break;
    case Act::DANCE:
        actEnd = now + 3200;
        break;
    case Act::BOUNCE:
        actEnd = now + 1800;
        break;
    case Act::YAWN:
        actEnd = now + 1600;
        break;
    case Act::CHASE:
        actEnd = now + 3000;
        break;
    default:
        actEnd = now + (unsigned long)rnd(1800, 4500);
        break;
    }
}

void PetView::ambient(unsigned long now, const Pet &pet)
{
    if ((long)(now - nextAmbient) < 0)
    {
        return;
    }
    float w = width(pet), top = headY(pet);
    nextAmbient = now + 1000;
    ambientCount++;
    switch (pet.mood())
    {
    case Mood::ASLEEP:
        particles.spawn(now, Bit::Z, x + w * 0.35f + (ambientCount % 2) * 6, top + 6, 14, -22, 2200,
                        ambientCount % 3 == 0 ? 14 : 10, SHINE, 0, true);
        nextAmbient = now + 1300;
        break;
    case Mood::SICK:
        particles.spawn(now, Bit::DROP, x + (facingLeft ? -1 : 1) * w * 0.42f, top + 8, 0, 25, 900, 8, WATER, 80);
        nextAmbient = now + 2600;
        break;
    case Mood::SAD:
        particles.spawn(now, Bit::DROP, x - FORM_RIG[pet.getForm()].eyeGap * 0.5f,
                        Room::GROUND - FORM_RIG[pet.getForm()].h * 0.5f, 0, 30, 800, 6, WATER, 120);
        nextAmbient = now + 3200;
        break;
    default:
        if (act == Act::DANCE)
        {
            particles.spawn(now, Bit::NOTE, x + rnd(-w * 0.6f, w * 0.6f), top - 4, rnd(-15, 15), -40, 1400, 12,
                            ambientCount % 2 ? GOLD : ACCENT);
            nextAmbient = now + 600;
        }
        break;
    }
}

// The timed moments inside a care animation: bites, sparkles, the egg's crack
void PetView::beats(unsigned long now, const Pet &pet)
{
    unsigned long t = now - animStart;
    float w = width(pet), top = headY(pet);
    switch (anim)
    {
    case Anim::MEAL:
    case Anim::SNACK:
        if (t >= 400 + animStep * 380u && animStep < (anim == Anim::MEAL ? 4 : 2))
        {
            animStep++;
            kick(-3);
            float mx = x + (facingLeft ? -1 : 1) * w * 0.3f;
            for (int i = 0; i < 2; i++)
            {
                particles.spawn(now, Bit::CRUMB, mx, Room::GROUND - FORM_RIG[pet.getForm()].h * 0.35f, rnd(-30, 30),
                                rnd(-60, -20), 600, 6, GOLD, 300);
            }
        }
        break;
    case Anim::CLEAN:
        if (t >= animStep * 180u)
        {
            animStep++;
            float broom = Room::X + 12 + t * (Room::W - 24) / (float)ANIM_MS[(uint8_t)Anim::CLEAN];
            particles.spawn(now, Bit::SPARKLE, broom - 6, Room::GROUND - rnd(2, 14), 0, -20, 700, 9, WATER);
        }
        break;
    case Anim::HATCH:
        if (animStep == 0 && t >= HATCH_OPEN_MS)
        {
            animStep = 1;
            burst(now, x, Room::GROUND - FORM_RIG[FORM_EGG].h - 6, 14, true);
            kick(-6);
            hop(now, 240, 60);
        }
        break;
    case Anim::EVOLVE:
        if (animStep == 0 && t >= EVOLVE_POP_MS)
        {
            animStep = 1;
            burst(now, x, top + FORM_RIG[pet.getForm()].h * 0.4f, 10, false);
            kick(6);
        }
        break;
    case Anim::CHEER:
        if (animStep == 0 && t >= 450)
        {
            animStep = 1;
            hop(now, 180);
        }
        break;
    default:
        break;
    }
}

void PetView::update(unsigned long now, const Pet &pet)
{
    float dt = (now - lastUpdate) / 1000.0f;
    lastUpdate = now;
    if (dt > 0.1f)
    {
        dt = 0.1f;
    }
    particles.update(now, dt);
    if (anim != Anim::NONE && !isPlaying(now))
    {
        anim = Anim::NONE;
    }
    beats(now, pet);

    // The lamp fades the room; its flicker starts when it comes on
    float dark = pet.isLightOn() ? 0.0f : 1.0f;
    if (dark < roomState.night && roomState.night >= 1.0f)
    {
        roomState.lampAt = now;
    }
    float fade = dt / 1.5f;
    roomState.night += fmaxf(-fade, fminf(fade, dark - roomState.night));
    roomState.stars = pet.isAsleep();

    // The spring, and gravity
    stretch.step(dt);
    if (jumpAt != 0 && (long)(now - jumpAt) >= 0)
    {
        jumpAt = 0;
        airborne = true;
        liftV = jumpPower;
        kick(5);
    }
    if (airborne)
    {
        liftV -= GRAVITY * dt;
        lift += liftV * dt;
        if (lift <= 0)
        {
            lift = 0;
            airborne = false;
            kick(-0.016f * fabsf(liftV) - 1);
        }
    }

    float w = width(pet);
    float lo = Room::WALK_LEFT + w * 0.5f, hi = Room::WALK_RIGHT - w * 0.5f;
    x = fmaxf(lo, fminf(hi > lo ? hi : lo, x));

    if (pet.isAsleep() != seenAsleep)
    {
        seenAsleep = pet.isAsleep();
        if (!seenAsleep)
        {
            play(Anim::WAKE, now, pet);
        }
    }
    if (pet.getFluff() > seenFluff && !pet.isAsleep() && !pet.isEgg() && !isPlaying(now))
    {
        play(Anim::STARTLE, now, pet);
    }
    seenFluff = pet.getFluff();

    if (pet.isEgg())
    {
        return;
    }
    ambient(now, pet);

    // Its eyes: glances, a held look at something, blinks
    if ((long)(now - lookUntil) < 0)
    {
        lookX += (lookToX - lookX) * fminf(1, dt * 10);
        lookY += (lookToY - lookY) * fminf(1, dt * 10);
    }
    else
    {
        if ((long)(now - nextGlance) >= 0)
        {
            nextGlance = now + (unsigned long)rnd(1500, 4000);
            lookToX = random(3) == 0 ? 0 : rnd(-0.8f, 0.8f);
            lookToY = rnd(-0.3f, 0.3f);
        }
        if (act == Act::CHASE)
        {
            lookToX = wave(now, 1400) * 0.9f;
            lookToY = -0.8f;
        }
        lookX += (lookToX - lookX) * fminf(1, dt * 6);
        lookY += (lookToY - lookY) * fminf(1, dt * 6);
    }
    if (pet.isAsleep())
    {
        return;
    }
    if ((long)(now - nextBlink) >= 0)
    {
        blinkUntil = now + 130;
        nextBlink = now + (unsigned long)rnd(2500, 6000);
    }

    if (isPlaying(now))
    {
        return;
    }
    if ((long)(now - actEnd) >= 0)
    {
        choose(now, pet);
    }
    float speed = pet.isSick() || pet.mood() == Mood::SLEEPY ? 18 : 34;
    switch (act)
    {
    case Act::WALK:
    case Act::SNIFF:
        if (fabsf(target - x) > 1)
        {
            facingLeft = target < x;
            x += (facingLeft ? -1 : 1) * fminf(speed * dt, fabsf(target - x));
            walkPhase += dt * speed / 5;
        }
        else if (act == Act::SNIFF)
        {
            facingLeft = false;
            roomState.pokeAt = (long)(now - roomState.pokeAt) > 1800 ? now : roomState.pokeAt;
        }
        else
        {
            walkPhase = 0;
        }
        break;
    case Act::DANCE:
        if (!airborne && jumpAt == 0 && (now - actStart) % 800 < 40)
        {
            hop(now, 150, 60);
        }
        break;
    case Act::BOUNCE:
    case Act::CHASE:
        if (!airborne && jumpAt == 0)
        {
            hop(now, act == Act::CHASE ? 130 : 200, 80);
        }
        break;
    default:
        walkPhase = 0;
        break;
    }
}

Pose PetView::pose(const Pet &pet, unsigned long now) const
{
    Pose p;
    const FormRig &rig = FORM_RIG[pet.getForm()];
    bool playing = isPlaying(now);
    unsigned long t = now - animStart, ta = now - actStart;
    Mood mood = pet.mood();
    p.x = x;
    p.ground = Room::GROUND;
    p.lift = lift;
    p.mirror = facingLeft;
    p.lookX = (facingLeft ? -1 : 1) * 0.2f + lookX;
    p.lookY = lookY;
    p.sway = wave(now, 2600);

    float breath = wave(now, pet.isAsleep() ? 3600 : 2300) * (pet.isAsleep() ? 0.05f : 0.03f);
    float s = fmaxf(-0.45f, fminf(0.6f, stretch.value)) + breath;
    p.squashY = 1 + s;
    p.squashX = 1 - s * 0.7f;
    if (rig.feature == Feature::CLOUD)
    {
        p.lift += 8 + wave(now, 2200) * 3;
    }

    // Walking: a bob a step and the feet alternate
    if ((act == Act::WALK || act == Act::SNIFF) && walkPhase > 0 && !playing && fabsf(target - x) > 1)
    {
        p.step = sinf(walkPhase * TAU);
        p.lift += fabsf(p.step) * 2.5f;
        p.lean = (facingLeft ? -1 : 1) * 2;
    }

    // The face by mood, then the act, then the animation over both
    p.blush = 0.4f;
    switch (mood)
    {
    case Mood::ASLEEP:
        p.eye = Eye::CLOSED;
        p.mouth = (now / 1800) % 2 ? Mouth::POUT : Mouth::NONE;
        p.squashY *= 0.84f;
        p.squashX *= 1.1f;
        p.blush = 0.5f;
        break;
    case Mood::SICK:
        p.eye = Eye::DIZZY;
        p.mouth = Mouth::FLAT;
        p.pale = true;
        p.lean = wave(now, 1800) * 3;
        p.blush = 0;
        break;
    case Mood::SLEEPY:
        p.eye = Eye::BLINK;
        p.mouth = Mouth::FLAT;
        break;
    case Mood::SAD:
        p.eye = Eye::SAD;
        p.mouth = Mouth::FROWN;
        p.blush = 0.15f;
        break;
    case Mood::MEH:
        p.mouth = Mouth::FLAT;
        break;
    case Mood::HAPPY:
        p.mouth = Mouth::W;
        p.blush = 0.9f;
        break;
    default:
        p.mouth = Mouth::SMILE;
        p.blush = 0.6f;
        break;
    }

    if (!playing && !pet.isAsleep())
    {
        switch (act)
        {
        case Act::SIT:
            p.squashY *= 0.9f;
            p.squashX *= 1.06f;
            break;
        case Act::YAWN:
        {
            float u = sinf(fminf(1, ta / 1400.0f) * (float)M_PI);
            p.squashY *= 1 + u * 0.12f;
            p.squashX *= 1 - u * 0.06f;
            if (u > 0.3f)
            {
                p.eye = Eye::CLOSED;
                p.mouth = Mouth::OPEN;
            }
            break;
        }
        case Act::DANCE:
            p.lean = wave(ta, 800) * 6;
            p.eye = Eye::HAPPY;
            p.mouth = Mouth::W;
            break;
        case Act::BOUNCE:
            p.eye = Eye::HAPPY;
            break;
        case Act::SNIFF:
            if (fabsf(target - x) <= 1)
            {
                p.lean = 5 + wave(now, 400) * 1.5f;
                p.eye = Eye::CLOSED;
                p.mouth = Mouth::POUT;
            }
            break;
        case Act::LOOK:
            p.lookX = wave(ta, 2400) * 0.9f;
            break;
        case Act::SIGH:
            p.squashY *= 0.94f;
            p.eye = mood == Mood::SAD ? Eye::SAD : Eye::BLINK;
            break;
        case Act::DOZE:
            p.eye = Eye::CLOSED;
            p.lean = wave(now, 2600) * 2;
            break;
        default:
            break;
        }
        if ((long)(now - blinkUntil) < 0 && (p.eye == Eye::OPEN || p.eye == Eye::SAD))
        {
            p.eye = Eye::BLINK;
        }
    }

    if (playing)
    {
        switch (anim)
        {
        case Anim::MEAL:
        case Anim::SNACK:
            p.eye = Eye::HAPPY;
            p.mouth = t > 350 && (t / 190) % 2 ? Mouth::OPEN : Mouth::CHEW;
            p.blush = 0.8f;
            p.lean = (facingLeft ? -1 : 1) * 3;
            break;
        case Anim::CUDDLE:
            p.eye = Eye::SQUEEZE;
            p.mouth = Mouth::W;
            p.blush = 1;
            p.lean = wave(t, 500) * 3;
            break;
        case Anim::CLEAN:
        case Anim::CHEER:
            p.eye = Eye::HAPPY;
            p.mouth = Mouth::W;
            p.blush = 0.9f;
            break;
        case Anim::MEDS:
        case Anim::STARTLE:
            p.eye = Eye::WIDE;
            p.mouth = Mouth::OPEN;
            break;
        case Anim::REFUSE:
            p.eye = Eye::BLINK;
            p.mouth = Mouth::POUT;
            p.puff = 1;
            p.blush = 0.9f;
            p.lean = wave(t, 260) * 5;
            break;
        case Anim::WAKE:
            p.eye = t < 400 ? Eye::BLINK : Eye::OPEN;
            p.mouth = Mouth::OPEN;
            break;
        case Anim::EVOLVE:
            p.eye = Eye::STAR;
            p.mouth = Mouth::OPEN;
            p.flash = t < EVOLVE_POP_MS ? 0.5f + 0.5f * wave(t, 220) : fmaxf(0, 1 - (t - EVOLVE_POP_MS) / 400.0f);
            p.blush = 1;
            break;
        case Anim::HATCH:
            p.eye = Eye::STAR;
            p.mouth = Mouth::W;
            p.blush = 1;
            break;
        default:
            break;
        }
    }
    return p;
}

void PetView::draw(Canvas &c, const Pet &pet, unsigned long now) const
{
    bool playing = isPlaying(now);
    unsigned long t = now - animStart;
    Pose p = pose(pet, now);
    FormId form = pet.getForm();
    const FormRig &rig = FORM_RIG[form];
    float w = rig.w;

    // The bowl, popping in with an overshoot, its food going a bite at a time
    if (playing && (anim == Anim::MEAL || anim == Anim::SNACK))
    {
        float u = fminf(1, t / 260.0f);
        float pop = u < 1 ? 1 + 0.35f * sinf(u * (float)M_PI) : 1.0f;
        if (t + 250 > ANIM_MS[(uint8_t)anim])
        {
            pop = fmaxf(0, (ANIM_MS[(uint8_t)anim] - t) / 250.0f);
        }
        float bx = x + (facingLeft ? -1 : 1) * (w * 0.5f + 20), by = Room::GROUND - 9;
        uint8_t left = (anim == Anim::MEAL ? 4 : 2) - animStep;
        if (anim == Anim::MEAL)
        {
            uint16_t food = Theme::rgb(GOLD);
            for (uint8_t i = 0; i < left; i++)
            {
                c.fillCircle(bx - 6 + i * 4, by - 3 - (i % 2) * 2, 4 * pop, food);
            }
            c.fillRoundRect(bx - 14 * pop, by - 2, 28 * pop, 11 * pop, 5 * pop, Theme::rgb(ACCENT));
            c.fillRoundRect(bx - 15 * pop, by - 3, 30 * pop, 4 * pop, 2, Canvas::mix(Theme::rgb(ACCENT), Theme::rgb(SHINE), 90));
        }
        else if (left > 0)
        {
            drawIcon(c, Icon::CANDY, bx, by - 2, 18 * pop * (left / 2.0f + 0.3f), Theme::rgb(BLUSH), Theme::rgb(SHINE));
        }
    }
    if (playing && anim == Anim::CLEAN)
    {
        float broom = Room::X + 12 + t * (Room::W - 24) / (float)ANIM_MS[(uint8_t)Anim::CLEAN];
        drawIcon(c, Icon::BROOM, broom, Room::GROUND - 12, 30, Theme::rgb(FLOOR_LINE), Theme::rgb(GOLD));
    }

    if (pet.isEgg() || (playing && anim == Anim::HATCH && t < HATCH_OPEN_MS))
    {
        Pose egg;
        egg.x = x;
        egg.ground = Room::GROUND;
        egg.eye = Eye::CLOSED;
        egg.mouth = Mouth::NONE;
        egg.blush = 0.6f;
        unsigned long every = pet.getSecsToGrow() < 30 ? 900 : 3500;
        bool shaking = playing ? t < HATCH_CRACK_MS : now % every < 500;
        if (shaking)
        {
            egg.lean = wave(now, 140) * (playing ? 6 : 4);
            egg.squashY = 1 + wave(now, 280) * 0.03f;
        }
        else
        {
            egg.squashY = 1 + wave(now, 2600) * 0.02f;
        }
        egg.squashX = 2 - egg.squashY;
        drawPet(c, FORM_EGG, egg);
        if (playing && t >= HATCH_CRACK_MS)
        {
            uint16_t ink = Theme::rgb(EYE);
            float cy = Room::GROUND - FORM_RIG[FORM_EGG].h * 0.55f;
            const float zig[5][2] = {{-18, 0}, {-9, -6}, {0, 1}, {9, -6}, {18, 0}};
            for (int i = 0; i < 4; i++)
            {
                c.line(x + zig[i][0], cy + zig[i][1], x + zig[i + 1][0], cy + zig[i + 1][1], 2, ink);
            }
        }
    }
    else
    {
        drawPet(c, form, p);
    }
    particles.draw(c, now, false);
}
