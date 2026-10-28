#ifndef STAGE_H
#define STAGE_H

#include <Arduino.h>
#include "Levels.h"

// One step of the simulation is 1/60 s, whatever the frame rate
static const uint8_t STEPS_PER_SECOND = 60;

struct Input
{
    bool left;
    bool right;
    bool jumpHeld;
    bool jumpPressed; // Since the last step
    bool dashPressed;
};

// Bits of Stage::takeEvents(), for the sounds
enum StageEvent : uint16_t
{
    EV_JUMP = 1 << 0,
    EV_STOMP = 1 << 1,
    EV_DASH = 1 << 2,
    EV_DIE = 1 << 3,
    EV_SHOT = 1 << 4,
    EV_CRACK = 1 << 5,
    EV_CHECKPOINT = 1 << 6,
    EV_BOSS_WAKE = 1 << 7,
    EV_BOSS_CHARGE = 1 << 8,
    EV_BOSS_THUD = 1 << 9,
    EV_BOSS_HIT = 1 << 10,
    EV_BOSS_DOWN = 1 << 11,
    EV_TALK = 1 << 12,
    EV_LAND = 1 << 13,
};

// What the renderer draws over the tiles, in order
struct SpriteDraw
{
    enum Kind : uint8_t
    {
        SHEET,
        SILHOUETTE, // The sheet frame in one flat colour: the dash's afterimages
        FLAG,       // A checkpoint, `frame` 1 once reached
    };
    Kind kind;
    const SheetDef *sheet;
    uint8_t frame;
    bool flip;
    int16_t x; // World position of the frame's top-left corner
    int16_t y;
    uint16_t color; // SILHOUETTE only, plain RGB565
};

class Stage
{
public:
    enum class Phase : uint8_t
    {
        PLAYING,
        TALKING, // Reached the coworker; the screen runs the dialogue, then calls finish()
        DONE
    };

    static const uint8_t MAX_COLS = 255;
    static const uint8_t MAX_ENEMIES = 16;
    static const uint8_t MAX_PRINTERS = 6;
    static const uint8_t MAX_PAPERS = 12;
    static const uint8_t MAX_CRUMBLES = 12;
    static const uint8_t MAX_CHECKPOINTS = 6;
    static const uint8_t MAX_DRAWS = 64;

    void load(uint8_t levelIndex);
    void step(const Input &input);
    void finish() { phase = Phase::DONE; }

    uint16_t takeEvents()
    {
        uint16_t e = events;
        events = 0;
        return e;
    }

    uint8_t getLevel() const { return level; }
    const LevelDef &getDef() const { return LEVELS[level]; }
    Phase getPhase() const { return phase; }
    uint16_t getWidth() const { return width; }
    Tile tileAt(int16_t col, int16_t row) const;
    // The art of a cell, TILE_ART_COUNT when it draws nothing (empty, open gates, printers)
    uint8_t artAt(int16_t col, int16_t row) const;
    int16_t getCameraX() const;
    uint32_t getTicks() const { return ticks; }
    uint16_t getDeaths() const { return deaths; }
    bool isBossFight() const { return fightStarted && !bossDefeated; }
    uint8_t getBossHitsLeft() const { return BOSS_HITS - boss.hits; }

    // Fills `out` with this step's sprites, back to front; returns how many
    uint8_t collectDraws(SpriteDraw *out, uint8_t max) const;

    // For the tests: where the player is, and whether he is standing on something
    float getPlayerX() const { return player.x; }
    float getPlayerY() const { return player.y; }
    float getPlayerVY() const { return player.vy; }
    bool isPlayerDead() const { return player.state == PlayerState::DEAD; }
    bool isPlayerOnFloor() const { return player.onFloor; }
    bool isBossDizzy() const { return boss.state == BossState::DIZZY; }
    bool isBossDefeated() const { return bossDefeated; }
    uint8_t aliveEnemies() const;

private:
    static const uint8_t BOSS_HITS = 3;

    enum class PlayerState : uint8_t
    {
        IDLE,
        RUN,
        JUMP,
        FALL,
        WALL_SLIDE,
        DASH,
        DEAD,
        TALK
    };

    struct Body
    {
        float x; // Hitbox top-left, world pixels
        float y;
        float vx;
        float vy;
        int16_t w;
        int16_t h;
        bool onFloor;
        bool hitWall;
        int8_t crumble; // The crumbling platform stood on, or -1
    };

    struct Player : Body
    {
        PlayerState state;
        int8_t facing;
        float coyote;
        float jumpBuffer;
        float wallJumpTime;
        float dashTime;
        bool hasDashed;
        bool wasJumpHeld;
        int8_t dashDir;
        uint16_t stateTicks;
        float prevBottom; // Feet before this step's move, to tell a stomp from a side hit
        float trailX[2];
        float trailY[2];
    };

    enum class EnemyKind : uint8_t
    {
        WALKER,
        DRUNK
    };

    struct Enemy : Body
    {
        EnemyKind kind;
        bool alive;
        uint8_t dying; // Ticks of the squash left once stomped
        int8_t dir; // 0 while pausing at an edge
        int8_t facing;
        int8_t nextDir;
        uint16_t pause;
        uint16_t moodTicks;
        float speed;
        float spawnX;
        float spawnY;
    };

    struct Printer
    {
        int16_t x; // Sprite top-left
        int16_t y;
        int8_t dir;
        uint16_t timer;
    };

    struct Paper
    {
        bool active;
        float x;
        float baseY;
        float y;
        float vx;
        uint16_t age;
    };

    enum class CrumbleState : uint8_t
    {
        SOLID,
        CRACKING,
        CRUMBLING,
        GONE
    };

    struct Crumble
    {
        int16_t x;
        int16_t top; // Walking surface
        uint8_t cells;
        CrumbleState state;
        uint16_t timer;
    };

    struct Checkpoint
    {
        int16_t x; // Centre
        int16_t bottom;
    };

    enum class BossState : uint8_t
    {
        WAIT,
        WAKE,
        PACE,
        TELEGRAPH,
        CHARGE,
        DIZZY,
        HURT,
        DOWN,
        GONE
    };

    struct Boss
    {
        bool present;
        BossState state;
        float x; // Hitbox top-left
        float y;
        int8_t dir;
        uint16_t timer;
        uint16_t anim;
        uint8_t hits;
        float spawnX;
    };

    uint8_t level = 0;
    uint16_t width = 0;
    uint8_t tiles[MAP_ROWS][MAX_COLS];
    uint8_t art[MAP_ROWS][MAX_COLS];
    Phase phase = Phase::PLAYING;
    uint32_t ticks = 0; // Play time, stopped while talking
    uint32_t clock = 0; // Every step, for the animations
    uint16_t deaths = 0;
    uint16_t events = 0;
    uint32_t rng = 1;

    Player player;
    float startX = 0;
    float startY = 0;
    int8_t checkpoint = -1;
    float camX = 0;
    uint8_t shake = 0;

    Enemy enemies[MAX_ENEMIES];
    uint8_t enemyCount = 0;
    Printer printers[MAX_PRINTERS];
    uint8_t printerCount = 0;
    Paper papers[MAX_PAPERS];
    Crumble crumbles[MAX_CRUMBLES];
    uint8_t crumbleCount = 0;
    Checkpoint checkpoints[MAX_CHECKPOINTS];
    uint8_t checkpointCount = 0;

    Boss boss;
    int16_t arenaLeft = 0; // World x of the entry gate's cell
    bool fightStarted = false;
    bool bossDefeated = false;

    int16_t womanX = -1;
    int16_t womanY = 0;
    int16_t machineX = -1;
    int16_t machineY = 0;

    uint32_t random(uint32_t bound);
    void autotile();
    void resetActors();

    bool isSolid(int16_t col, int16_t row) const;
    bool isDesk(int16_t col, int16_t row) const;
    bool isSpike(int16_t col, int16_t row) const;
    bool solidAt(float x, float y) const;
    void moveBody(Body &body);
    bool overlaps(const Body &body, float x, float y, float w, float h) const;
    int16_t exitX() const;

    void stepPlayer(const Input &input);
    void jump();
    void bounce(bool held);
    bool wallAhead() const;
    void kill();
    void stepEnemies();
    void stepPrinters();
    void stepCrumbles();
    void stepBoss();
    void touchHazards(const Input &input);
    float cameraTarget() const;
    void clampCamera();
    void stepCamera();
};

#endif // STAGE_H
