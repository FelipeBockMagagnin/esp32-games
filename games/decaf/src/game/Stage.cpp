#include "Stage.h"
#include <math.h>

static const float DT = 1.0f / STEPS_PER_SECOND;

// From the Godot prototype (jogo-cafe, entities/player/player.gd), drawn at the same pixel scale
static const float ACCELERATION = 3000;
static const float MAX_SPEED = 150;
static const float FALL_LIMIT = 400;
static const float GRAVITY = 1500;
static const float JUMP_VELOCITY = -400;
static const float BOUNCE_VELOCITY = -600;
static const float COYOTE_TIME = 0.1f;
static const float JUMP_BUFFER_TIME = 0.16f;
static const float WALL_JUMP_X = 150;
static const float WALL_JUMP_TIME = 0.16f;
static const float DASH_SPEED = 300;
static const float DASH_TIME = 0.15f;
static const float WALL_SLIDE_FACTOR = 0.4f;
// Not in the prototype: letting go of jump cuts the rise short, and a stomp without jump held
// is only a hop, so the high bounce is a choice
static const float JUMP_CUT_VELOCITY = -260;
static const float HOP_VELOCITY = -330;
static const float DEATH_POP_VELOCITY = -260;

static const int16_t PLAYER_W = 12;
static const int16_t PLAYER_H = 22;
static const int16_t WALKER_W = 16;
static const int16_t WALKER_H = 24;
static const float WALKER_SPEED = 60;
static const uint16_t EDGE_PAUSE = 30;
static const float STOMP_MARGIN = 8; // How far below an enemy's top the feet may come and still stomp
static const uint8_t SQUASH_TICKS = 18;
static const uint16_t DEATH_TICKS = 50;

static const uint16_t PRINT_PERIOD = 150;
static const uint16_t PRINT_WARNING = 30;
static const int16_t PRINTER_RANGE = 300;
static const float PAPER_SPEED = 80;
static const uint16_t PAPER_LIFE = 480;

static const uint8_t CRUMBLE_SURFACE = 7; // The platform art's top row inside its 16 px frame
static const uint16_t CRACK_TICKS = 32;
static const uint16_t CRUMBLE_FALL_TICKS = 50;
static const uint16_t CRUMBLE_RESPAWN_TICKS = 180;

static const int16_t BOSS_W = 28;
static const int16_t BOSS_H = 46;
static const int16_t BOSS_BODY_X = 36; // The body's centre in the 64 px frame, facing right
static const float BOSS_WALK = 45;
static const float BOSS_CHARGE = 170;
static const float BOSS_CHARGE_STEP = 40; // Faster with every hit taken
static const uint16_t BOSS_WAKE_TICKS = 48;
static const uint16_t BOSS_PACE_TICKS = 90;
static const uint16_t BOSS_TELEGRAPH_TICKS = 36;
static const uint16_t BOSS_DIZZY_TICKS = 100;
static const uint16_t BOSS_HURT_TICKS = 45;
static const uint16_t BOSS_DOWN_TICKS = 90;

static const float CAMERA_LEAD = 40;
static const float CAMERA_EASE = 0.12f;

static const uint16_t TRAIL_COLOR = 0xF892; // The prototype's dash trail, Color(1, 0.08, 0.58)

static float moveToward(float from, float to, float delta)
{
    if (from < to)
    {
        return from + delta < to ? from + delta : to;
    }
    return from - delta > to ? from - delta : to;
}

static int16_t cellOf(float v)
{
    return (int16_t)floorf(v / TILE);
}

uint32_t Stage::random(uint32_t bound)
{
    rng = rng * 1103515245u + 12345u;
    return bound == 0 ? 0 : (rng >> 16) % bound;
}

void Stage::load(uint8_t levelIndex)
{
    level = levelIndex < LEVEL_COUNT ? levelIndex : 0;
    const LevelDef &def = LEVELS[level];

    width = 0;
    for (uint8_t r = 0; r < MAP_ROWS; r++)
    {
        size_t len = strlen(def.map[r]);
        if (len > width)
        {
            width = len < MAX_COLS ? len : MAX_COLS;
        }
    }

    memset(tiles, 0, sizeof(tiles));
    enemyCount = printerCount = crumbleCount = checkpointCount = 0;
    boss = Boss();
    womanX = machineX = -1;
    arenaLeft = 0;
    startX = TILE;
    startY = 0;

    for (uint8_t r = 0; r < MAP_ROWS; r++)
    {
        const char *row = def.map[r];
        size_t len = strlen(row);
        for (uint16_t c = 0; c < width && c < len; c++)
        {
            int16_t x = c * TILE;
            int16_t bottom = (r + 1) * TILE;
            switch (row[c])
            {
            case '#':
                tiles[r][c] = (uint8_t)Tile::GROUND;
                break;
            case '=':
                tiles[r][c] = (uint8_t)Tile::BLOCK;
                break;
            case '-':
                tiles[r][c] = (uint8_t)Tile::DESK;
                break;
            case 'X':
                tiles[r][c] = (uint8_t)Tile::CABINET;
                break;
            case '^':
                tiles[r][c] = (uint8_t)Tile::SPIKE_UP;
                break;
            case 'v':
                tiles[r][c] = (uint8_t)Tile::SPIKE_DOWN;
                break;
            case 'A':
                tiles[r][c] = (uint8_t)Tile::ENTRY_GATE;
                arenaLeft = x;
                break;
            case 'E':
                tiles[r][c] = (uint8_t)Tile::EXIT_GATE;
                break;
            case 'S':
                startX = x + (TILE - PLAYER_W) / 2;
                startY = bottom - PLAYER_H;
                break;
            case 'C':
                if (checkpointCount < MAX_CHECKPOINTS)
                {
                    checkpoints[checkpointCount++] = {(int16_t)(x + TILE / 2), bottom};
                }
                break;
            case 'W':
            case 'D':
                if (enemyCount < MAX_ENEMIES)
                {
                    Enemy &e = enemies[enemyCount++];
                    e = Enemy();
                    e.kind = row[c] == 'W' ? EnemyKind::WALKER : EnemyKind::DRUNK;
                    e.spawnX = x + (TILE - WALKER_W) / 2;
                    e.spawnY = bottom - WALKER_H;
                }
                break;
            case 'P':
            case 'Q':
                if (printerCount < MAX_PRINTERS && r > 0 && c + 1 < MAX_COLS)
                {
                    printers[printerCount++] = {x, (int16_t)(bottom - 2 * TILE), (int8_t)(row[c] == 'P' ? -1 : 1), 0};
                    tiles[r][c] = tiles[r][c + 1] = (uint8_t)Tile::PRINTER;
                    tiles[r - 1][c] = tiles[r - 1][c + 1] = (uint8_t)Tile::PRINTER;
                }
                break;
            case 'B':
            case 'b':
                if (crumbleCount < MAX_CRUMBLES)
                {
                    crumbles[crumbleCount++] = {x, (int16_t)(r * TILE + CRUMBLE_SURFACE),
                                                (uint8_t)(row[c] == 'B' ? 3 : 1), CrumbleState::SOLID, 0};
                }
                break;
            case 'K':
                boss.present = true;
                boss.spawnX = x + TILE / 2 - BOSS_W / 2;
                boss.y = bottom - BOSS_H;
                break;
            case 'F':
                womanX = x - TILE / 2;
                womanY = bottom - 32;
                break;
            case 'M':
                machineX = x - TILE / 2;
                machineY = bottom - 32;
                break;
            }
        }
    }
    autotile();

    phase = Phase::PLAYING;
    ticks = 0;
    clock = 0;
    deaths = 0;
    events = 0;
    rng = 12345u + level;
    checkpoint = -1;
    fightStarted = false;
    bossDefeated = false;
    resetActors();
}

static bool sameKind(const uint8_t tiles[MAP_ROWS][Stage::MAX_COLS], uint16_t width, int16_t c, int16_t r,
                     Tile kind)
{
    // Off the map counts as more of the same, so the edges of the screen show no rim
    if (c < 0 || c >= width || r < 0 || r >= MAP_ROWS)
    {
        return true;
    }
    return tiles[r][c] == (uint8_t)kind;
}

void Stage::autotile()
{
    for (int16_t r = 0; r < MAP_ROWS; r++)
    {
        for (int16_t c = 0; c < width; c++)
        {
            Tile t = (Tile)tiles[r][c];
            uint8_t a = TILE_ART_COUNT;
            if (t == Tile::GROUND || t == Tile::CABINET)
            {
                bool up = sameKind(tiles, width, c, r - 1, t);
                bool down = sameKind(tiles, width, c, r + 1, t);
                bool left = sameKind(tiles, width, c - 1, r, t);
                bool right = sameKind(tiles, width, c + 1, r, t);
                bool ground = t == Tile::GROUND;
                uint8_t mix = (uint8_t)((c * 7 + r * 3) % 4);
                if (!up)
                {
                    a = !left    ? (ground ? TILE_ART_GROUND_TL : TILE_ART_CABINET_TL)
                        : !right ? (ground ? TILE_ART_GROUND_TR : TILE_ART_CABINET_TR)
                                 : (ground ? TILE_ART_GROUND_T0 + mix : TILE_ART_CABINET_T);
                }
                else if (!down)
                {
                    a = !left    ? (ground ? TILE_ART_GROUND_BL : TILE_ART_CABINET_BL)
                        : !right ? (ground ? TILE_ART_GROUND_BR : TILE_ART_CABINET_BR)
                                 : (ground ? TILE_ART_GROUND_B : TILE_ART_CABINET_B);
                }
                else
                {
                    a = !left    ? (ground ? TILE_ART_GROUND_L : TILE_ART_CABINET_L)
                        : !right ? (ground ? TILE_ART_GROUND_R : TILE_ART_CABINET_R)
                                 : (ground ? TILE_ART_GROUND_M0 + mix % 2 : TILE_ART_CABINET_M);
                }
            }
            else if (t == Tile::DESK)
            {
                bool left = c > 0 && tiles[r][c - 1] == (uint8_t)Tile::DESK;
                bool right = c + 1 < width && tiles[r][c + 1] == (uint8_t)Tile::DESK;
                a = !left && right ? TILE_ART_DESK_L : left && !right ? TILE_ART_DESK_R : TILE_ART_DESK_M;
            }
            else if (t == Tile::BLOCK)
            {
                a = TILE_ART_BLOCK;
            }
            else if (t == Tile::SPIKE_UP)
            {
                a = TILE_ART_SPIKE_UP;
            }
            else if (t == Tile::SPIKE_DOWN)
            {
                a = TILE_ART_SPIKE_DOWN;
            }
            else if (t == Tile::ENTRY_GATE || t == Tile::EXIT_GATE)
            {
                a = TILE_ART_POST;
            }
            art[r][c] = a;
        }
    }
}

void Stage::resetActors()
{
    player = Player();
    player.w = PLAYER_W;
    player.h = PLAYER_H;
    player.facing = 1;
    player.crumble = -1;
    if (checkpoint >= 0)
    {
        player.x = checkpoints[checkpoint].x - PLAYER_W / 2;
        player.y = checkpoints[checkpoint].bottom - PLAYER_H;
    }
    else
    {
        player.x = startX;
        player.y = startY;
    }
    player.prevBottom = player.y + player.h;

    for (uint8_t i = 0; i < enemyCount; i++)
    {
        Enemy &e = enemies[i];
        e.x = e.spawnX;
        e.y = e.spawnY;
        e.vx = e.vy = 0;
        e.w = WALKER_W;
        e.h = WALKER_H;
        e.alive = true;
        e.dying = 0;
        e.dir = e.facing = -1;
        e.nextDir = -1;
        e.pause = 0;
        e.speed = WALKER_SPEED;
        e.moodTicks = 30 + random(60);
        e.crumble = -1;
    }
    for (uint8_t i = 0; i < printerCount; i++)
    {
        printers[i].timer = 40 + i * 37 % PRINT_PERIOD;
    }
    for (Paper &p : papers)
    {
        p.active = false;
    }
    for (uint8_t i = 0; i < crumbleCount; i++)
    {
        crumbles[i].state = CrumbleState::SOLID;
        crumbles[i].timer = 0;
    }
    if (boss.present && !bossDefeated)
    {
        boss.state = BossState::WAIT;
        boss.x = boss.spawnX;
        boss.dir = -1;
        boss.timer = boss.anim = 0;
        boss.hits = 0;
        fightStarted = false;
    }
    shake = 0;
    camX = cameraTarget();
    clampCamera();
}

Tile Stage::tileAt(int16_t col, int16_t row) const
{
    if (col < 0 || col >= width || row < 0 || row >= MAP_ROWS)
    {
        return Tile::EMPTY;
    }
    return (Tile)tiles[row][col];
}

uint8_t Stage::artAt(int16_t col, int16_t row) const
{
    if (col < 0 || col >= width || row < 0 || row >= MAP_ROWS)
    {
        return TILE_ART_COUNT;
    }
    Tile t = (Tile)tiles[row][col];
    if ((t == Tile::ENTRY_GATE || t == Tile::EXIT_GATE) && !isSolid(col, row))
    {
        return TILE_ART_COUNT;
    }
    return art[row][col];
}

bool Stage::isSolid(int16_t col, int16_t row) const
{
    if (col < 0 || col >= width)
    {
        return true;
    }
    if (row < 0 || row >= MAP_ROWS)
    {
        return false;
    }
    switch ((Tile)tiles[row][col])
    {
    case Tile::GROUND:
    case Tile::BLOCK:
    case Tile::CABINET:
    case Tile::PRINTER:
        return true;
    case Tile::ENTRY_GATE:
        return fightStarted && !bossDefeated;
    case Tile::EXIT_GATE:
        return !bossDefeated;
    default:
        return false;
    }
}

bool Stage::isDesk(int16_t col, int16_t row) const
{
    return tileAt(col, row) == Tile::DESK;
}

bool Stage::isSpike(int16_t col, int16_t row) const
{
    Tile t = tileAt(col, row);
    return t == Tile::SPIKE_UP || t == Tile::SPIKE_DOWN;
}

bool Stage::solidAt(float x, float y) const
{
    return isSolid(cellOf(x), cellOf(y));
}

bool Stage::overlaps(const Body &body, float x, float y, float w, float h) const
{
    return body.x < x + w && body.x + body.w > x && body.y < y + h && body.y + body.h > y;
}

void Stage::moveBody(Body &b)
{
    b.hitWall = false;
    float nx = b.x + b.vx * DT;
    int16_t r0 = cellOf(b.y);
    int16_t r1 = cellOf(b.y + b.h - 0.01f);
    if (b.vx > 0)
    {
        int16_t col = cellOf(nx + b.w - 0.01f);
        for (int16_t r = r0; r <= r1; r++)
        {
            if (isSolid(col, r))
            {
                nx = col * TILE - b.w;
                b.vx = 0;
                b.hitWall = true;
                break;
            }
        }
    }
    else if (b.vx < 0)
    {
        int16_t col = cellOf(nx);
        for (int16_t r = r0; r <= r1; r++)
        {
            if (isSolid(col, r))
            {
                nx = (col + 1) * TILE;
                b.vx = 0;
                b.hitWall = true;
                break;
            }
        }
    }
    b.x = nx;

    float prevBottom = b.y + b.h;
    float ny = b.y + b.vy * DT;
    int16_t c0 = cellOf(b.x);
    int16_t c1 = cellOf(b.x + b.w - 0.01f);
    b.onFloor = false;
    b.crumble = -1;
    if (b.vy >= 0)
    {
        int16_t row = cellOf(ny + b.h);
        for (int16_t c = c0; c <= c1; c++)
        {
            if (isSolid(c, row) || (isDesk(c, row) && prevBottom <= row * TILE + 0.01f))
            {
                ny = row * TILE - b.h;
                b.vy = 0;
                b.onFloor = true;
                break;
            }
        }
        for (uint8_t i = 0; i < crumbleCount && !b.onFloor; i++)
        {
            const Crumble &cr = crumbles[i];
            if (cr.state != CrumbleState::SOLID && cr.state != CrumbleState::CRACKING)
            {
                continue;
            }
            if (b.x + b.w > cr.x && b.x < cr.x + cr.cells * TILE && prevBottom <= cr.top + 0.01f &&
                ny + b.h >= cr.top)
            {
                ny = cr.top - b.h;
                b.vy = 0;
                b.onFloor = true;
                b.crumble = i;
            }
        }
    }
    else
    {
        int16_t row = cellOf(ny);
        for (int16_t c = c0; c <= c1; c++)
        {
            if (isSolid(c, row))
            {
                ny = (row + 1) * TILE;
                b.vy = 0;
                break;
            }
        }
    }
    b.y = ny;
}

void Stage::step(const Input &input)
{
    clock++;
    if (phase != Phase::PLAYING)
    {
        return;
    }
    ticks++;
    stepPlayer(input);
    stepEnemies();
    stepPrinters();
    stepCrumbles();
    stepBoss();
    touchHazards(input);
    stepCamera();
    if (shake > 0)
    {
        shake--;
    }
}

void Stage::jump()
{
    player.vy = JUMP_VELOCITY;
    player.coyote = 0;
    player.jumpBuffer = 0;
    events |= EV_JUMP;
}

void Stage::bounce(bool held)
{
    player.vy = held ? BOUNCE_VELOCITY : HOP_VELOCITY;
    player.state = PlayerState::JUMP;
    player.coyote = 0;
    player.jumpBuffer = 0;
    player.hasDashed = false;
    events |= EV_STOMP;
}

bool Stage::wallAhead() const
{
    float probe = player.facing > 0 ? player.x + player.w + 1 : player.x - 1;
    return solidAt(probe, player.y + player.h / 2);
}

void Stage::kill()
{
    if (player.state == PlayerState::DEAD)
    {
        return;
    }
    player.state = PlayerState::DEAD;
    player.stateTicks = 0;
    player.vx = 0;
    player.vy = DEATH_POP_VELOCITY;
    deaths++;
    events |= EV_DIE;
}

void Stage::stepPlayer(const Input &input)
{
    Player &p = player;
    p.stateTicks++;
    if (p.state == PlayerState::DEAD)
    {
        // Pops up and falls through everything, then the floor restarts at the checkpoint
        p.y += p.vy * DT;
        p.vy += GRAVITY * DT;
        if (p.stateTicks >= DEATH_TICKS)
        {
            resetActors();
        }
        return;
    }

    p.coyote -= DT;
    p.jumpBuffer -= DT;
    p.wallJumpTime -= DT;
    if (input.jumpPressed)
    {
        p.jumpBuffer = JUMP_BUFFER_TIME;
    }
    if (p.wasJumpHeld && !input.jumpHeld && p.vy < JUMP_CUT_VELOCITY && p.state != PlayerState::DASH)
    {
        p.vy = JUMP_CUT_VELOCITY;
    }
    p.wasJumpHeld = input.jumpHeld;
    int8_t axis = (int8_t)(input.right ? 1 : 0) - (int8_t)(input.left ? 1 : 0);

    auto friction = [&]() { p.vx = moveToward(p.vx, 0, ACCELERATION * DT); };
    auto horizontal = [&]()
    {
        if (axis != 0)
        {
            p.vx = moveToward(p.vx, MAX_SPEED * axis, ACCELERATION * DT);
            p.facing = axis;
        }
        else
        {
            friction();
        }
    };
    auto gravity = [&]()
    {
        if (p.vy < FALL_LIMIT)
        {
            p.vy += GRAVITY * DT;
        }
    };
    auto setState = [&](PlayerState s)
    {
        p.state = s;
        p.stateTicks = 0;
    };
    auto tryDash = [&]()
    {
        if (p.hasDashed || !input.dashPressed)
        {
            return false;
        }
        p.hasDashed = true;
        p.dashTime = DASH_TIME;
        p.dashDir = axis != 0 ? axis : p.facing;
        p.facing = p.dashDir;
        p.vx = p.dashDir * DASH_SPEED;
        p.vy = 0;
        for (uint8_t i = 0; i < 2; i++)
        {
            p.trailX[i] = p.x;
            p.trailY[i] = p.y;
        }
        setState(PlayerState::DASH);
        events |= EV_DASH;
        return true;
    };
    auto tryGroundJump = [&]()
    {
        if ((p.onFloor || p.coyote > 0) && p.jumpBuffer > 0)
        {
            jump();
            setState(PlayerState::JUMP);
            return true;
        }
        return false;
    };
    auto tryWallJump = [&]()
    {
        if (!p.onFloor && wallAhead() && p.jumpBuffer > 0)
        {
            p.vy = JUMP_VELOCITY;
            p.vx = -WALL_JUMP_X * p.facing;
            p.facing = -p.facing;
            p.wallJumpTime = WALL_JUMP_TIME;
            p.jumpBuffer = 0;
            events |= EV_JUMP;
            setState(PlayerState::JUMP);
            return true;
        }
        return false;
    };
    auto tryWallSlide = [&]()
    {
        if (!p.onFloor && wallAhead() && p.vy >= 0)
        {
            setState(PlayerState::WALL_SLIDE);
            return true;
        }
        return false;
    };

    switch (p.state)
    {
    case PlayerState::IDLE:
    case PlayerState::RUN:
        if (p.onFloor && p.vy >= 0)
        {
            p.hasDashed = false;
        }
        p.coyote = COYOTE_TIME;
        if (p.state == PlayerState::IDLE)
        {
            friction();
        }
        else
        {
            horizontal();
        }
        gravity();
        if (tryDash() || tryGroundJump())
        {
            break;
        }
        if (!p.onFloor)
        {
            setState(PlayerState::FALL);
        }
        else if (p.state == PlayerState::IDLE && axis != 0)
        {
            setState(PlayerState::RUN);
        }
        else if (p.state == PlayerState::RUN && axis == 0 && p.vx == 0)
        {
            setState(PlayerState::IDLE);
        }
        break;
    case PlayerState::JUMP:
    case PlayerState::FALL:
        if (p.wallJumpTime <= 0)
        {
            horizontal();
        }
        gravity();
        if (tryDash() || tryWallJump() || tryWallSlide())
        {
            break;
        }
        if (p.state == PlayerState::JUMP)
        {
            if (p.vy >= 0)
            {
                setState(PlayerState::FALL);
            }
            break;
        }
        if (tryGroundJump())
        {
            break;
        }
        if (p.onFloor)
        {
            setState(axis != 0 ? PlayerState::RUN : PlayerState::IDLE);
        }
        break;
    case PlayerState::WALL_SLIDE:
        gravity();
        p.vy *= WALL_SLIDE_FACTOR;
        if (!wallAhead() || (axis != 0 && axis != p.facing))
        {
            horizontal();
        }
        if (tryWallJump() || tryDash())
        {
            break;
        }
        if (p.onFloor)
        {
            setState(PlayerState::IDLE);
        }
        else if (!wallAhead())
        {
            setState(PlayerState::FALL);
        }
        break;
    case PlayerState::DASH:
        p.dashTime -= DT;
        p.vx = p.dashDir * DASH_SPEED;
        p.vy = 0;
        if (p.stateTicks % 3 == 0)
        {
            p.trailX[1] = p.trailX[0];
            p.trailY[1] = p.trailY[0];
            p.trailX[0] = p.x;
            p.trailY[0] = p.y;
        }
        if (p.dashTime <= 0)
        {
            setState(p.onFloor ? (axis != 0 ? PlayerState::RUN : PlayerState::IDLE) : PlayerState::FALL);
        }
        break;
    case PlayerState::TALK:
        friction();
        gravity();
        break;
    case PlayerState::DEAD:
        break;
    }

    bool wasOnFloor = p.onFloor;
    p.prevBottom = p.y + p.h;
    moveBody(p);
    if (p.onFloor && !wasOnFloor)
    {
        events |= EV_LAND;
    }
    if (p.onFloor && p.crumble >= 0 && crumbles[p.crumble].state == CrumbleState::SOLID)
    {
        crumbles[p.crumble].state = CrumbleState::CRACKING;
        crumbles[p.crumble].timer = 0;
        events |= EV_CRACK;
    }
    if (p.y > VIEW_H + TILE)
    {
        kill();
        return;
    }

    for (int8_t i = checkpointCount - 1; i > checkpoint; i--)
    {
        if (p.x + p.w / 2 >= checkpoints[i].x && p.onFloor)
        {
            checkpoint = i;
            events |= EV_CHECKPOINT;
            break;
        }
    }

    if (womanX >= 0 && p.onFloor && p.state != PlayerState::TALK && p.x + p.w >= womanX + 2)
    {
        setState(PlayerState::TALK);
        p.facing = 1;
        phase = Phase::TALKING;
        events |= EV_TALK;
    }
}

void Stage::stepEnemies()
{
    for (uint8_t i = 0; i < enemyCount; i++)
    {
        Enemy &e = enemies[i];
        if (!e.alive)
        {
            continue;
        }
        if (e.dying > 0)
        {
            if (--e.dying == 0)
            {
                e.alive = false;
            }
            continue;
        }

        if (e.kind == EnemyKind::DRUNK && e.dir != 0 && --e.moodTicks == 0)
        {
            e.moodTicks = 40 + random(80);
            if (random(100) < 25)
            {
                e.nextDir = random(2) ? e.dir : -e.dir;
                e.dir = 0;
                e.pause = 20 + random(40);
            }
            else
            {
                e.speed = 20 + random(70);
            }
        }

        if (e.dir == 0)
        {
            if (e.pause > 0)
            {
                e.pause--;
            }
            if (e.pause == 0)
            {
                e.dir = e.facing = e.nextDir;
            }
        }
        else if (e.onFloor)
        {
            // The prototype's rays: ground under the leading foot, and no wall at the waist. Spikes
            // count as an edge, so a walker never paces into them.
            int16_t col = cellOf(e.dir > 0 ? e.x + e.w + 1 : e.x - 1);
            int16_t below = cellOf(e.y + e.h + 1);
            bool ground = isSolid(col, below) || isDesk(col, below);
            bool wall = isSolid(col, cellOf(e.y + e.h / 2));
            bool spikes = isSpike(col, below) || isSpike(col, cellOf(e.y + e.h - 1));
            if (!ground || wall || spikes)
            {
                e.nextDir = -e.dir;
                e.dir = 0;
                e.pause = EDGE_PAUSE;
            }
        }

        float target = e.dir * e.speed;
        e.vx += (target - e.vx) * 10 * DT;
        if (e.vy < FALL_LIMIT)
        {
            e.vy += GRAVITY * DT;
        }
        moveBody(e);
        if (e.hitWall && e.dir != 0)
        {
            e.nextDir = -e.dir;
            e.dir = 0;
            e.pause = EDGE_PAUSE;
        }
        if (e.y > VIEW_H + TILE)
        {
            e.alive = false;
        }
    }
}

void Stage::stepPrinters()
{
    float playerCentre = player.x + player.w / 2;
    for (uint8_t i = 0; i < printerCount; i++)
    {
        Printer &pr = printers[i];
        if (fabsf(playerCentre - (pr.x + TILE)) > PRINTER_RANGE)
        {
            continue;
        }
        if (++pr.timer < PRINT_PERIOD)
        {
            continue;
        }
        pr.timer = 0;
        for (Paper &paper : papers)
        {
            if (!paper.active)
            {
                paper.active = true;
                paper.x = pr.dir < 0 ? pr.x - 6 : pr.x + 2 * TILE;
                paper.baseY = paper.y = pr.y + 12;
                paper.vx = pr.dir * PAPER_SPEED;
                paper.age = 0;
                events |= EV_SHOT;
                break;
            }
        }
    }

    for (Paper &paper : papers)
    {
        if (!paper.active)
        {
            continue;
        }
        paper.age++;
        paper.x += paper.vx * DT;
        paper.y = paper.baseY + 4 * sinf(paper.age * (2 * (float)M_PI / STEPS_PER_SECOND));
        if (paper.age > PAPER_LIFE || solidAt(paper.x + 3, paper.y + 2))
        {
            paper.active = false;
        }
    }
}

void Stage::stepCrumbles()
{
    for (uint8_t i = 0; i < crumbleCount; i++)
    {
        Crumble &cr = crumbles[i];
        switch (cr.state)
        {
        case CrumbleState::SOLID:
            break;
        case CrumbleState::CRACKING:
            if (++cr.timer >= CRACK_TICKS)
            {
                cr.state = CrumbleState::CRUMBLING;
                cr.timer = 0;
            }
            break;
        case CrumbleState::CRUMBLING:
            if (++cr.timer >= CRUMBLE_FALL_TICKS)
            {
                cr.state = CrumbleState::GONE;
                cr.timer = 0;
            }
            break;
        case CrumbleState::GONE:
            // Back only once nobody stands where it would appear
            if (++cr.timer >= CRUMBLE_RESPAWN_TICKS &&
                !overlaps(player, cr.x, cr.top - 2, cr.cells * TILE, TILE))
            {
                cr.state = CrumbleState::SOLID;
                cr.timer = 0;
            }
            break;
        }
    }
}

void Stage::stepBoss()
{
    if (!boss.present || boss.state == BossState::GONE)
    {
        return;
    }
    boss.anim++;
    boss.timer++;
    float minX = arenaLeft + TILE;
    float maxX = exitX() - BOSS_W;
    float playerCentre = player.x + player.w / 2;
    auto enter = [&](BossState s)
    {
        boss.state = s;
        boss.timer = 0;
    };

    switch (boss.state)
    {
    case BossState::WAIT:
        if (player.state != PlayerState::DEAD && player.x > arenaLeft + 3 * TILE)
        {
            fightStarted = true;
            shake = 8;
            events |= EV_BOSS_WAKE;
            enter(BossState::WAKE);
        }
        break;
    case BossState::WAKE:
        if (boss.timer >= BOSS_WAKE_TICKS)
        {
            enter(BossState::PACE);
        }
        break;
    case BossState::PACE:
        boss.dir = playerCentre < boss.x + BOSS_W / 2 ? -1 : 1;
        boss.x += boss.dir * BOSS_WALK * DT;
        if (boss.timer >= BOSS_PACE_TICKS - boss.hits * 20)
        {
            boss.dir = playerCentre < boss.x + BOSS_W / 2 ? -1 : 1;
            enter(BossState::TELEGRAPH);
        }
        break;
    case BossState::TELEGRAPH:
        if (boss.timer >= BOSS_TELEGRAPH_TICKS)
        {
            events |= EV_BOSS_CHARGE;
            enter(BossState::CHARGE);
        }
        break;
    case BossState::CHARGE:
        boss.x += boss.dir * (BOSS_CHARGE + boss.hits * BOSS_CHARGE_STEP) * DT;
        if (boss.x <= minX || boss.x >= maxX)
        {
            shake = 10;
            events |= EV_BOSS_THUD;
            enter(BossState::DIZZY);
        }
        break;
    case BossState::DIZZY:
        if (boss.timer >= BOSS_DIZZY_TICKS)
        {
            enter(BossState::PACE);
        }
        break;
    case BossState::HURT:
        if (boss.timer >= BOSS_HURT_TICKS)
        {
            enter(BossState::PACE);
        }
        break;
    case BossState::DOWN:
        if (boss.timer >= BOSS_DOWN_TICKS)
        {
            bossDefeated = true;
            shake = 6;
            enter(BossState::GONE);
        }
        break;
    case BossState::GONE:
        break;
    }
    if (boss.x < minX)
    {
        boss.x = minX;
    }
    if (boss.x > maxX)
    {
        boss.x = maxX;
    }
}

int16_t Stage::exitX() const
{
    for (int16_t c = arenaLeft / TILE + 1; c < width; c++)
    {
        for (uint8_t r = 0; r < MAP_ROWS; r++)
        {
            if (tiles[r][c] == (uint8_t)Tile::EXIT_GATE)
            {
                return c * TILE;
            }
        }
    }
    return width * TILE;
}

void Stage::touchHazards(const Input &input)
{
    Player &p = player;
    if (p.state == PlayerState::DEAD || p.state == PlayerState::TALK)
    {
        return;
    }

    for (int16_t r = cellOf(p.y); r <= cellOf(p.y + p.h - 0.01f); r++)
    {
        for (int16_t c = cellOf(p.x); c <= cellOf(p.x + p.w - 0.01f); c++)
        {
            Tile t = tileAt(c, r);
            // The points only: the lower or upper half of the cell, less a sliver at each side
            if ((t == Tile::SPIKE_UP && overlaps(p, c * TILE + 2, r * TILE + 8, TILE - 4, 8)) ||
                (t == Tile::SPIKE_DOWN && overlaps(p, c * TILE + 2, r * TILE, TILE - 4, 8)))
            {
                kill();
                return;
            }
        }
    }

    for (const Paper &paper : papers)
    {
        if (paper.active && overlaps(p, paper.x, paper.y, 6, 5))
        {
            kill();
            return;
        }
    }

    bool fromAbove = p.vy > 0;
    for (uint8_t i = 0; i < enemyCount; i++)
    {
        Enemy &e = enemies[i];
        if (!e.alive || e.dying > 0 || !overlaps(p, e.x, e.y, e.w, e.h))
        {
            continue;
        }
        if (fromAbove && p.prevBottom <= e.y + STOMP_MARGIN)
        {
            e.dying = SQUASH_TICKS;
            bounce(input.jumpHeld);
            return;
        }
        kill();
        return;
    }

    bool dangerous = boss.state == BossState::PACE || boss.state == BossState::TELEGRAPH ||
                     boss.state == BossState::CHARGE || boss.state == BossState::DIZZY;
    if (boss.present && dangerous && overlaps(p, boss.x, boss.y, BOSS_W, BOSS_H))
    {
        if (fromAbove && p.prevBottom <= boss.y + STOMP_MARGIN + 4)
        {
            if (boss.state == BossState::DIZZY)
            {
                boss.hits++;
                boss.state = boss.hits >= BOSS_HITS ? BossState::DOWN : BossState::HURT;
                boss.timer = 0;
                events |= boss.hits >= BOSS_HITS ? EV_BOSS_DOWN : EV_BOSS_HIT;
                bounce(input.jumpHeld);
            }
            else
            {
                // Awake, the head only throws him off
                bounce(false);
            }
            return;
        }
        kill();
    }
}

float Stage::cameraTarget() const
{
    if (fightStarted && !bossDefeated)
    {
        return arenaLeft;
    }
    return player.x + player.w / 2 + player.facing * CAMERA_LEAD - VIEW_W / 2;
}

void Stage::clampCamera()
{
    float maxX = (float)width * TILE - VIEW_W;
    if (camX > maxX)
    {
        camX = maxX;
    }
    if (camX < 0)
    {
        camX = 0;
    }
}

void Stage::stepCamera()
{
    if (player.state != PlayerState::DEAD)
    {
        camX += (cameraTarget() - camX) * CAMERA_EASE;
    }
    clampCamera();
}

int16_t Stage::getCameraX() const
{
    int16_t x = (int16_t)camX;
    if (shake > 0)
    {
        x += (shake & 2) ? 2 : -2;
    }
    return x;
}

uint8_t Stage::aliveEnemies() const
{
    uint8_t n = 0;
    for (uint8_t i = 0; i < enemyCount; i++)
    {
        if (enemies[i].alive && enemies[i].dying == 0)
        {
            n++;
        }
    }
    return n;
}

uint8_t Stage::collectDraws(SpriteDraw *out, uint8_t max) const
{
    uint8_t n = 0;
    auto add = [&](SpriteDraw::Kind kind, const SheetDef *sheet, uint8_t frame, bool flip, float x, float y)
    {
        if (n < max)
        {
            out[n++] = {kind, sheet, frame, flip, (int16_t)floorf(x), (int16_t)floorf(y), 0};
        }
    };

    for (uint8_t i = 0; i < checkpointCount; i++)
    {
        add(SpriteDraw::FLAG, nullptr, i <= checkpoint ? 1 : 0, false, checkpoints[i].x, checkpoints[i].bottom);
    }
    for (uint8_t i = 0; i < crumbleCount; i++)
    {
        const Crumble &cr = crumbles[i];
        uint8_t frame = cr.state == CrumbleState::CRACKING ? cr.timer / (CRACK_TICKS / 4)
                        : cr.state == CrumbleState::CRUMBLING ? 4 + cr.timer / (CRUMBLE_FALL_TICKS / 10)
                                                             : 0;
        if (cr.state != CrumbleState::GONE)
        {
            const SheetDef *sheet = cr.cells == 3 ? &sheet_crumble : &sheet_crumble_small;
            add(SpriteDraw::SHEET, sheet, frame < sheet->count ? frame : sheet->count - 1, false, cr.x,
                cr.top - CRUMBLE_SURFACE);
        }
    }
    for (uint8_t i = 0; i < printerCount; i++)
    {
        const Printer &pr = printers[i];
        uint8_t frame = pr.timer >= PRINT_PERIOD - PRINT_WARNING ? 1 : pr.timer < 20 ? 2 : 0;
        // The art faces right
        add(SpriteDraw::SHEET, &sheet_printer, frame, pr.dir < 0, pr.x, pr.y);
    }
    if (machineX >= 0)
    {
        add(SpriteDraw::SHEET, &sheet_coffee, clock % 90 < 8 ? 1 : 0, false, machineX, machineY);
    }
    if (womanX >= 0)
    {
        uint8_t frame = phase == Phase::TALKING ? (clock / 10) % 2 : 0;
        add(SpriteDraw::SHEET, &sheet_woman, frame, false, womanX, womanY);
    }

    for (uint8_t i = 0; i < enemyCount; i++)
    {
        const Enemy &e = enemies[i];
        if (!e.alive)
        {
            continue;
        }
        // The art faces left; the sprite's feet sit on the bottom of its 32 px frame
        const SheetDef *sheet = e.kind == EnemyKind::WALKER ? &sheet_walker : &sheet_drunk;
        uint8_t frame;
        if (e.dying > 0)
        {
            if (e.kind == EnemyKind::DRUNK && (e.dying / 3) % 2)
            {
                continue;
            }
            frame = e.kind == EnemyKind::WALKER ? 6 + (SQUASH_TICKS - e.dying) / (SQUASH_TICKS / 3) : 0;
        }
        else if (e.dir == 0)
        {
            frame = e.kind == EnemyKind::WALKER ? 9 : 0;
        }
        else
        {
            frame = (uint8_t)(((clock + i * 5) / (e.kind == EnemyKind::WALKER ? 6 : 8)) % 6);
        }
        add(SpriteDraw::SHEET, sheet, frame, e.facing > 0, e.x - 8, e.y + e.h - 32);
    }

    if (boss.present && boss.state != BossState::GONE)
    {
        bool hidden = (boss.state == BossState::HURT || boss.state == BossState::DOWN) && (boss.timer / 4) % 2;
        if (!hidden)
        {
            uint8_t frame = 0;
            switch (boss.state)
            {
            case BossState::WAIT:
                frame = (clock / 30) % 2;
                break;
            case BossState::WAKE:
                frame = 2 + boss.timer * 4 / (BOSS_WAKE_TICKS + 1);
                break;
            case BossState::PACE:
                frame = 6 + (boss.anim / 6) % 8;
                break;
            case BossState::TELEGRAPH:
                frame = 14 + boss.timer * 3 / (BOSS_TELEGRAPH_TICKS + 1);
                break;
            case BossState::CHARGE:
                frame = 6 + (boss.anim / 3) % 8;
                break;
            case BossState::DIZZY:
                frame = 17 + (boss.anim / 8) % 3;
                break;
            default:
                frame = 17;
                break;
            }
            bool flip = boss.dir < 0;
            float left = boss.x + BOSS_W / 2 - (flip ? 64 - BOSS_BODY_X : BOSS_BODY_X);
            add(SpriteDraw::SHEET, LEVELS[level].boss, frame, flip, left, boss.y + BOSS_H - 64);
        }
    }

    for (const Paper &paper : papers)
    {
        if (paper.active)
        {
            add(SpriteDraw::SHEET, &sheet_paper, (paper.age / 6) % 5, paper.vx > 0, paper.x - 1, paper.y - 2);
        }
    }

    const Player &p = player;
    float left = p.x + p.w / 2 - 16;
    float top = p.y + p.h - 27;
    if (p.state == PlayerState::DASH)
    {
        for (uint8_t i = 0; i < 2; i++)
        {
            if (n < max)
            {
                out[n++] = {SpriteDraw::SILHOUETTE, &sheet_player, 5, p.facing < 0,
                            (int16_t)floorf(p.trailX[i] + p.w / 2 - 16), (int16_t)floorf(p.trailY[i] + p.h - 27),
                            TRAIL_COLOR};
            }
        }
    }
    static const uint8_t RUN_FRAMES[] = {11, 12, 13, 12};
    uint8_t frame = 0;
    switch (p.state)
    {
    case PlayerState::IDLE:
    case PlayerState::TALK:
        frame = (clock / 40) % 3 == 2 ? 16 : 0;
        break;
    case PlayerState::RUN:
        frame = RUN_FRAMES[(clock / 6) % 4];
        break;
    case PlayerState::JUMP:
        frame = p.vy < -150 ? 6 : 7;
        break;
    case PlayerState::FALL:
        frame = 8;
        break;
    case PlayerState::WALL_SLIDE:
        frame = 9;
        break;
    case PlayerState::DASH:
        frame = 5;
        break;
    case PlayerState::DEAD:
        frame = 1;
        break;
    }
    add(SpriteDraw::SHEET, &sheet_player, frame, p.facing < 0, left, top);
    return n;
}
