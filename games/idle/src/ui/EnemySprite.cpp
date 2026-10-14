#include "EnemySprite.h"
#include "Theme.h"

static const int16_t CARD_W = 76;
static const int16_t CARD_H = 40;

static const unsigned long FRAME_MS = 33;

// Frame pixels, so each step is SCALE screen pixels
static const unsigned long SHAKE_STEP_MS = 30;
static const int8_t LIGHT_SHAKE[] = {1, -1, 1, -1};
static const int8_t HEAVY_SHAKE[] = {3, -3, 2, -2, 1, -1};
static const uint8_t LIGHT_STEPS = sizeof(LIGHT_SHAKE);
static const uint8_t HEAVY_STEPS = sizeof(HEAVY_SHAKE);
static const unsigned long LIGHT_FLASH_MS = 70;
static const unsigned long HEAVY_FLASH_MS = 110;
static const uint8_t HIT_FLASH_ALPHA = 170;

static const unsigned long DEATH_FLASH_MS = 90;
static const unsigned long DEATH_MS = 520;
static const uint8_t DEATH_FLASH_ALPHA = 230;
static const int16_t DEATH_SINK = 6;

static const unsigned long BOB_MS = 500;

static const unsigned long SPAWN_MS = 320;
static const int16_t SPAWN_DROP = 8;

// Frame pixels, from the middle of the frame
static const unsigned long CAST_MS = 450;
static const int16_t CAST_RADIUS_MIN = 4;
static const int16_t CAST_RADIUS_MAX = 34;
static const int16_t CAST_THICKNESS = 3;
static const uint8_t CAST_ALPHA = 220;

static const unsigned long LUNGE_STEP_MS = 35;
static const int8_t LUNGE_OFFSETS[] = {2, 4, 4, 3, 1}; // Frame pixels down, toward the player's side
static const uint8_t LUNGE_STEPS = sizeof(LUNGE_OFFSETS);

static const unsigned long NUMBER_MS = 750;
static const unsigned long NUMBER_BLINK_MS = 150; // Blinks out over the end of its life
static const int16_t NUMBER_START_Y = 26;
static const int16_t NUMBER_RISE = 18;
static const int16_t NUMBER_SPREAD = 8;

// 3x5 digits, one byte per row, the high of three bits on the left
static const uint8_t DIGIT_W = 3;
static const uint8_t DIGIT_H = 5;
static const uint8_t DIGITS[10][DIGIT_H] PROGMEM = {
    {7, 5, 5, 5, 7}, {2, 6, 2, 2, 7}, {7, 1, 7, 4, 7}, {7, 1, 7, 1, 7}, {5, 5, 7, 1, 1},
    {7, 4, 7, 1, 7}, {7, 4, 7, 5, 7}, {7, 1, 1, 1, 1}, {7, 5, 7, 5, 7}, {7, 5, 7, 1, 7},
};

static uint16_t swapBytes(uint16_t color)
{
    return (uint16_t)((color >> 8) | (color << 8));
}

// alphaBlend on byte-swapped words, as the line buffer and the palettes hold them
static uint16_t blendSwapped(uint8_t alpha, uint16_t fg, uint16_t bg)
{
    fg = swapBytes(fg);
    bg = swapBytes(bg);
    uint16_t inv = 255 - alpha;
    uint16_t r = (((fg >> 11) & 0x1F) * alpha + ((bg >> 11) & 0x1F) * inv) / 255;
    uint16_t g = (((fg >> 5) & 0x3F) * alpha + ((bg >> 5) & 0x3F) * inv) / 255;
    uint16_t b = ((fg & 0x1F) * alpha + (bg & 0x1F) * inv) / 255;
    return swapBytes((uint16_t)((r << 11) | (g << 5) | b));
}

struct RowStyle
{
    int16_t dx;        // Frame pixels
    uint8_t tintAlpha; // Toward `tint`, 0 for none
    uint16_t tint;     // Byte-swapped
    uint8_t opacity;   // Over what the line already holds
};

static const RowStyle PLAIN = {0, 0, 0, 255};

// Paints the opaque pixels of one frame row of `def` into `line`, scaled, leaving the
// transparent ones as they were. `runs` walks the encoded rows in order, so it must be
// called for every frame row, top to bottom; rows above the sprite may be negative.
static void paintRow(const SpriteDef &def, const uint8_t *&runs, int16_t row, uint16_t *line,
                     const RowStyle &style)
{
    if (row < def.y || row >= def.y + def.h)
    {
        return;
    }

    // Runs never cross a row, so this row's runs add up to exactly w pixels
    int16_t px = (def.x + style.dx) * EnemySprite::SCALE;
    for (uint8_t col = 0; col < def.w;)
    {
        uint8_t length = pgm_read_byte(runs++);
        uint8_t index = pgm_read_byte(runs++);
        col += length;
        if (index == 0)
        {
            px += length * EnemySprite::SCALE;
            continue;
        }
        uint16_t color = pgm_read_word(def.palette + index - 1);
        if (style.tintAlpha > 0)
        {
            color = blendSwapped(style.tintAlpha, style.tint, color);
        }
        for (int16_t k = length * EnemySprite::SCALE; k > 0; k--, px++)
        {
            if (px >= 0 && px < EnemySprite::SIZE)
            {
                line[px] = style.opacity == 255 ? color : blendSwapped(style.opacity, color, line[px]);
            }
        }
    }
}

// Walks `runs` past one encoded row without painting it, for rows lifted above the frame
static void skipRow(const SpriteDef &def, const uint8_t *&runs)
{
    for (uint8_t col = 0; col < def.w; runs += 2)
    {
        col += pgm_read_byte(runs);
    }
}

EnemySprite::EnemySprite(int16_t x, int16_t y, const SpriteDef *sprite)
    : UI(x, y, SIZE, SIZE), sprite(sprite), background(nullptr), countdown(0),
      dyingSprite(nullptr), deathStart(0), hitting(false), hitHeavy(false), hitStart(0),
      lunging(false), lungeStart(0), spawning(false), spawnStart(0),
      casting(false), castInward(false), castColor(0), castStart(0),
      nextNumber(0), bob(0), now(0), lastFrame(0), animating(false)
{
    for (uint8_t i = 0; i < MAX_NUMBERS; i++)
    {
        numbers[i].active = false;
    }
}

void EnemySprite::hit(bool heavy)
{
    if (dyingSprite != nullptr)
    {
        return;
    }
    hitting = true;
    hitHeavy = heavy;
    hitStart = now;
    lastFrame = 0;
}

void EnemySprite::lunge()
{
    if (dyingSprite != nullptr || sprite == nullptr)
    {
        return;
    }
    lunging = true;
    lungeStart = now;
    lastFrame = 0;
}

void EnemySprite::addDamage(uint32_t damage, bool heavy)
{
    DamageNumber &number = numbers[nextNumber];
    nextNumber = (nextNumber + 1) % MAX_NUMBERS;

    uint8_t digits = 1;
    for (uint32_t v = damage; v >= 10; v /= 10)
    {
        digits++;
    }
    int16_t scale = heavy ? 2 : 1;
    int16_t width = digits * (DIGIT_W + 1) * scale - scale;
    int16_t left = (SPRITE_FRAME - width) / 2 + random(-NUMBER_SPREAD, NUMBER_SPREAD + 1);
    left = constrain(left, 1, SPRITE_FRAME - width - 1);

    number.active = true;
    number.heavy = heavy;
    number.start = now;
    number.x = left;
    number.value = damage;
    lastFrame = 0;
}

void EnemySprite::kill()
{
    if (sprite == nullptr)
    {
        return;
    }
    dyingSprite = sprite;
    deathStart = now;
    hitting = false;
    lastFrame = 0;
}

void EnemySprite::spawn()
{
    if (sprite == nullptr)
    {
        return;
    }
    dyingSprite = nullptr;
    spawning = true;
    spawnStart = now;
    lastFrame = 0;
}

void EnemySprite::cast(uint16_t color, bool inward)
{
    casting = true;
    castInward = inward;
    castColor = swapBytes(color);
    castStart = now;
    lastFrame = 0;
}

void EnemySprite::stopAnimations()
{
    dyingSprite = nullptr;
    hitting = false;
    lunging = false;
    spawning = false;
    casting = false;
    for (uint8_t i = 0; i < MAX_NUMBERS; i++)
    {
        numbers[i].active = false;
    }
    animating = false;
    markDirty();
}

void EnemySprite::update(unsigned long time)
{
    now = time;

    if (dyingSprite != nullptr && now - deathStart >= DEATH_MS)
    {
        dyingSprite = nullptr;
    }
    unsigned long hitMs = hitHeavy ? SHAKE_STEP_MS * HEAVY_STEPS : SHAKE_STEP_MS * LIGHT_STEPS;
    if (hitting && now - hitStart >= hitMs)
    {
        hitting = false;
    }

    if (lunging && now - lungeStart >= LUNGE_STEP_MS * LUNGE_STEPS)
    {
        lunging = false;
    }
    if (spawning && now - spawnStart >= SPAWN_MS)
    {
        spawning = false;
    }
    if (casting && now - castStart >= CAST_MS)
    {
        casting = false;
    }

    bool anyNumber = false;
    for (uint8_t i = 0; i < MAX_NUMBERS; i++)
    {
        if (numbers[i].active && now - numbers[i].start >= NUMBER_MS)
        {
            numbers[i].active = false;
        }
        anyNumber = anyNumber || numbers[i].active;
    }

    bool wasAnimating = animating;
    animating = dyingSprite != nullptr || hitting || lunging || spawning || casting || anyNumber;
    if (animating && now - lastFrame >= FRAME_MS)
    {
        lastFrame = now;
        markDirty();
    }
    else if (wasAnimating && !animating)
    {
        markDirty();
    }

    uint8_t newBob = sprite != nullptr && dyingSprite == nullptr ? (now / BOB_MS) % 2 : 0;
    if (bob != newBob)
    {
        bob = newBob;
        markDirty();
    }
}

// A dark shadow one frame pixel down and right, then the digits, so they read over any art
void EnemySprite::paintNumbers(uint8_t row, uint16_t *line) const
{
    static const uint16_t SHADOW = swapBytes(Theme::HEADER);

    for (uint8_t i = 0; i < MAX_NUMBERS; i++)
    {
        const DamageNumber &number = numbers[i];
        if (!number.active)
        {
            continue;
        }
        unsigned long age = now - number.start;
        if (age > NUMBER_MS - NUMBER_BLINK_MS && (age / (NUMBER_BLINK_MS / 3)) % 2 == 1)
        {
            continue;
        }

        int16_t scale = number.heavy ? 2 : 1;
        int16_t top = NUMBER_START_Y - (int32_t)NUMBER_RISE * age / NUMBER_MS - (scale - 1) * DIGIT_H;
        uint16_t color = swapBytes(number.heavy ? Theme::GOLD : Theme::TEXT);

        char text[11];
        snprintf(text, sizeof(text), "%lu", (unsigned long)number.value);

        for (uint8_t pass = 0; pass < 2; pass++)
        {
            int16_t offset = pass == 0 ? 1 : 0;
            int16_t glyphRow = (int16_t)row - top - offset;
            if (glyphRow < 0 || glyphRow >= DIGIT_H * scale)
            {
                continue;
            }
            glyphRow /= scale;

            int16_t left = number.x + offset;
            for (const char *c = text; *c != '\0'; c++, left += (DIGIT_W + 1) * scale)
            {
                uint8_t bits = pgm_read_byte(&DIGITS[*c - '0'][glyphRow]);
                for (uint8_t col = 0; col < DIGIT_W; col++)
                {
                    if ((bits & (4 >> col)) == 0)
                    {
                        continue;
                    }
                    for (int16_t s = 0; s < scale; s++)
                    {
                        int16_t fx = left + col * scale + s;
                        if (fx < 0 || fx >= SPRITE_FRAME)
                        {
                            continue;
                        }
                        for (uint8_t k = 0; k < SCALE; k++)
                        {
                            line[fx * SCALE + k] = pass == 0 ? SHADOW : color;
                        }
                    }
                }
            }
        }
    }
}

void EnemySprite::paintRing(uint8_t row, uint16_t *line) const
{
    if (!casting)
    {
        return;
    }
    unsigned long age = now - castStart;
    int16_t travel = (int32_t)(CAST_RADIUS_MAX - CAST_RADIUS_MIN) * age / CAST_MS;
    int16_t outer = castInward ? CAST_RADIUS_MAX - travel : CAST_RADIUS_MIN + travel;
    int16_t inner = outer - CAST_THICKNESS;
    int16_t dy = (int16_t)row - SPRITE_FRAME / 2;
    if (dy < -outer || dy > outer)
    {
        return;
    }
    uint8_t alpha = (uint8_t)(CAST_ALPHA - (uint32_t)CAST_ALPHA * age / CAST_MS);

    int16_t outerHalf = (int16_t)sqrtf((float)(outer * outer - dy * dy));
    int16_t innerHalf = inner > 0 && dy * dy < inner * inner ? (int16_t)sqrtf((float)(inner * inner - dy * dy)) : -1;
    for (int16_t side = -1; side <= 1; side += 2)
    {
        for (int16_t d = innerHalf + 1; d <= outerHalf; d++)
        {
            int16_t fx = SPRITE_FRAME / 2 + side * d - (side < 0 ? 1 : 0);
            if (fx < 0 || fx >= SPRITE_FRAME)
            {
                continue;
            }
            for (uint8_t k = 0; k < SCALE; k++)
            {
                uint16_t &px = line[fx * SCALE + k];
                px = blendSwapped(alpha, castColor, px);
            }
        }
    }
}

void EnemySprite::draw(TFT_eSPI &tft)
{
    const SpriteDef *enemy = dyingSprite != nullptr ? dyingSprite : sprite;
    RowStyle style = PLAIN;
    int16_t dy = bob;

    if (dyingSprite != nullptr)
    {
        unsigned long age = now - deathStart;
        if (age < DEATH_FLASH_MS)
        {
            style.tintAlpha = DEATH_FLASH_ALPHA;
        }
        else
        {
            unsigned long fade = DEATH_MS - DEATH_FLASH_MS;
            unsigned long t = age - DEATH_FLASH_MS < fade ? age - DEATH_FLASH_MS : fade;
            style.opacity = (uint8_t)(255 - 255 * t / fade);
            dy = (int16_t)(DEATH_SINK * t / fade);
        }
    }
    else if (hitting)
    {
        unsigned long age = now - hitStart;
        uint8_t step = age / SHAKE_STEP_MS;
        if (hitHeavy)
        {
            style.dx = step < HEAVY_STEPS ? HEAVY_SHAKE[step] : 0;
        }
        else
        {
            style.dx = step < LIGHT_STEPS ? LIGHT_SHAKE[step] : 0;
        }
        if (age < (hitHeavy ? HEAVY_FLASH_MS : LIGHT_FLASH_MS))
        {
            style.tintAlpha = HIT_FLASH_ALPHA;
        }
    }
    else if (spawning)
    {
        unsigned long age = now - spawnStart < SPAWN_MS ? now - spawnStart : SPAWN_MS;
        style.opacity = (uint8_t)(255 * age / SPAWN_MS);
        dy -= (int16_t)(SPAWN_DROP - (int32_t)SPAWN_DROP * age / SPAWN_MS);
    }
    if (lunging && dyingSprite == nullptr)
    {
        uint8_t step = (now - lungeStart) / LUNGE_STEP_MS;
        dy += step < LUNGE_STEPS ? LUNGE_OFFSETS[step] : 0;
    }
    style.tint = swapBytes(Theme::TEXT);

    // Palette words are byte-swapped like every other asset, so the fill colour is too
    uint16_t fill = swapBytes(eraseColor);
    uint16_t line[SIZE];
    const uint8_t *enemyRuns = enemy != nullptr ? enemy->runs : nullptr;
    const uint8_t *backgroundRuns = background != nullptr ? background->runs : nullptr;
    if (enemy != nullptr)
    {
        for (int16_t row = enemy->y; row < -dy && row < enemy->y + enemy->h; row++)
        {
            skipRow(*enemy, enemyRuns);
        }
    }

    tft.startWrite();
    tft.setAddrWindow(x, y, SIZE, SIZE);
    for (uint8_t row = 0; row < SPRITE_FRAME; row++)
    {
        for (int16_t i = 0; i < SIZE; i++)
        {
            line[i] = fill;
        }
        if (background != nullptr)
        {
            paintRow(*background, backgroundRuns, row, line, PLAIN);
        }
        if (enemy != nullptr)
        {
            paintRow(*enemy, enemyRuns, (int16_t)row - dy, line, style);
        }
        paintRing(row, line);
        paintNumbers(row, line);

        for (uint8_t k = 0; k < SCALE; k++)
        {
            tft.pushPixels(line, SIZE);
        }
    }
    tft.endWrite();

    // Painted after the blit, so it would flicker under a running animation
    if (!animating)
    {
        drawCountdown(tft);
    }
}

void EnemySprite::drawCountdown(TFT_eSPI &tft)
{
    if (countdown == 0)
    {
        return;
    }

    int16_t cx = x + w / 2;
    int16_t top = y + (h - CARD_H) / 2;
    tft.fillRoundRect(cx - CARD_W / 2, top, CARD_W, CARD_H, 4, Theme::CARD);
    tft.drawRoundRect(cx - CARD_W / 2, top, CARD_W, CARD_H, 4, Theme::FRAME);

    char buf[6];
    snprintf(buf, sizeof(buf), "%us", (unsigned)countdown);
    tft.setTextDatum(TC_DATUM);
    tft.setTextSize(1);
    tft.setTextColor(Theme::TEXT_2);
    tft.drawString("NEXT ENEMY", cx, top + 6);
    tft.setTextSize(2);
    tft.setTextColor(Theme::TEXT);
    tft.drawString(buf, cx, top + 19);
}
