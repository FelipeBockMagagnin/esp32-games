// Runs the real screens on the PC through scripted scenes and saves what the panel would show
// as a PNG per scene, a small bot doing the playing. Build and run with tools/shots.sh [out_dir].
#include <string>
#include "Png.h"
#include "../src/core/ScreenManager.h"
#include "../src/core/SoundManager.h"
#include "../src/screens/ConfirmScreen.h"
#include "../src/screens/PlayScreen.h"
#include "../src/screens/SettingsScreen.h"
#include "../src/screens/TitleScreen.h"
#include "../src/stack/Game.h"
#include "../src/stack/Records.h"
#include "../src/ui/Themes.h"

long random(long max) { return max > 0 ? rand() % max : 0; }
long random(long min, long max) { return min + random(max - min); }

static uint32_t state = 0xC0FFEEu;
static uint32_t source()
{
    state ^= state << 13;
    state ^= state >> 17;
    state ^= state << 5;
    return state;
}

static bool held[5] = {false, false, false, false, false};
static bool keyDown(Key key) { return held[(uint8_t)key]; }

static TFT_eSPI tft;
static Records records;
static Game game;
static Settings settings = {0, 1, 0};
static SoundManager sound(0);
static ScreenManager screens(tft);
static void nothing() {}
static void applyTheme()
{
    Theme::setActive(settings.theme);
    settings.theme = Theme::getActive();
}
static ConfirmScreen resetScreen({"Reset", "Reset everything?", "Every best run and setting", "will be erased.",
                                  "Resetting..."},
                                 Theme::BAD, nothing, screens, sound);
static PlayScreen playScreen(tft, game, records, source, nothing, keyDown, screens, sound);
static TitleScreen titleScreen(tft, records, nothing, settings, applyTheme, screens, sound, playScreen);
static SettingsScreen settingsScreen(settings, nothing, nothing, screens, sound, resetScreen);

static std::string outDir = ".";

static void run(unsigned long ms)
{
    for (unsigned long t = 0; t < ms; t += 16)
    {
        stubNow += 16;
        screens.update(stubNow);
    }
}

static void shot(const std::string &name)
{
    writePng(outDir + "/" + name + ".png", tft.pixels.data(), TFT_WIDTH, TFT_HEIGHT);
}

static int heightOf(uint8_t x)
{
    for (uint8_t y = 0; y < ROWS; y++)
    {
        if (game.cell(x, y) != 0)
        {
            return ROWS - y;
        }
    }
    return 0;
}

// How bad the stack looks with the piece dropped there: tall columns, holes under them, and a
// bumpy top. Lower is better; a very high number means the placement does not fit.
static long placementCost(uint8_t rot, int8_t x)
{
    Piece p = {game.piece().type, rot, x, game.piece().y};
    if (!game.fits(p))
    {
        return 1L << 30;
    }
    while (true)
    {
        Piece below = p;
        below.y++;
        if (!game.fits(below))
        {
            break;
        }
        p = below;
    }
    Cell cells[PIECE_CELLS];
    Game::cellsOf(p.type, p.rot, cells);
    uint8_t saved[PIECE_CELLS];
    for (uint8_t i = 0; i < PIECE_CELLS; i++)
    {
        saved[i] = game.cell(p.x + cells[i].x, p.y + cells[i].y);
        game.setCell(p.x + cells[i].x, p.y + cells[i].y, p.type + 1);
    }
    long cost = 0;
    int last = -1;
    for (uint8_t cx = 0; cx < COLS; cx++)
    {
        int h = heightOf(cx);
        cost += h * h;
        for (uint8_t y = ROWS - h; y < ROWS; y++)
        {
            cost += game.cell(cx, y) == 0 ? 60 : 0;
        }
        if (last >= 0)
        {
            cost += abs(h - last) * 4;
        }
        last = h;
    }
    for (uint8_t y = 0; y < ROWS; y++)
    {
        bool full = true;
        for (uint8_t cx = 0; cx < COLS && full; cx++)
        {
            full = game.cell(cx, y) != 0;
        }
        cost -= full ? 400 : 0;
    }
    for (uint8_t i = 0; i < PIECE_CELLS; i++)
    {
        game.setCell(p.x + cells[i].x, p.y + cells[i].y, saved[i]);
    }
    return cost;
}

// Plays one piece through the same key presses a player would use
static void playPiece()
{
    long best = 1L << 30;
    uint8_t bestRot = 0;
    int8_t bestX = game.piece().x;
    for (uint8_t rot = 0; rot < 4; rot++)
    {
        for (int8_t x = -2; x < COLS; x++)
        {
            long cost = placementCost(rot, x);
            if (cost < best)
            {
                best = cost;
                bestRot = rot;
                bestX = x;
            }
        }
    }
    for (uint8_t i = game.piece().rot; i != bestRot; i = (i + 1) & 3)
    {
        screens.handleConfirmPress();
        run(16);
    }
    for (uint8_t guard = 0; game.piece().x != bestX && guard < COLS * 2; guard++)
    {
        int8_t before = game.piece().x;
        if (game.piece().x < bestX)
        {
            screens.handleRightPress();
        }
        else
        {
            screens.handleBackPress();
        }
        run(16);
        if (game.piece().x == before)
        {
            break;
        }
    }
    screens.handleUpPress();
    run(16);
}

static void waitForPiece()
{
    for (int guard = 0; guard < 400 && !game.falling() && !game.over(); guard++)
    {
        run(16);
    }
}

static void playPieces(uint16_t count)
{
    for (uint16_t i = 0; i < count && !game.over(); i++)
    {
        waitForPiece();
        if (game.falling())
        {
            playPiece();
        }
    }
    waitForPiece();
}

static void start(Mode mode)
{
    records.mode = (uint8_t)mode;
    screens.setScreen(0);
    screens.handleConfirmPress();
    run(50);
}

static void openPause()
{
    held[(uint8_t)Key::CONFIRM] = true;
    screens.handleConfirmPress();
    run(800);
    held[(uint8_t)Key::CONFIRM] = false;
    run(32);
}

static void quit()
{
    openPause();
    screens.handleDownPress();
    screens.handleDownPress();
    screens.handleConfirmPress();
    run(100);
}

// A well with a loose lump above a nearly full floor, so the next long bar sets off a chain
static void setUpCascade()
{
    for (uint8_t y = 0; y < ROWS; y++)
    {
        for (uint8_t x = 0; x < COLS; x++)
        {
            game.setCell(x, y, 0);
        }
    }
    for (uint8_t y = ROWS - 4; y < ROWS; y++)
    {
        for (uint8_t x = 0; x < COLS; x++)
        {
            if (x != 5 && !(y == ROWS - 4 && x == 8))
            {
                game.setCell(x, y, (uint8_t)(1 + (x + y) % PIECE_COUNT));
            }
        }
    }
    game.setCell(8, ROWS - 7, 3);
    game.setCell(8, ROWS - 8, 3);
    game.setCell(8, ROWS - 9, 3);
    game.setPiece({0, 1, 3, 8});
}

int main(int argc, char **argv)
{
    if (argc > 1)
    {
        outDir = argv[1];
    }
    defaultRecords(records);
    records.best[0] = 1284;
    records.plays[0] = 23;
    records.best[1] = 1436;
    records.plays[1] = 8;
    records.best[2] = 2170;
    records.plays[2] = 12;
    stubNow = 1000;
    screens.addScreen(&titleScreen, "Empilha");
    screens.addScreen(&settingsScreen, "Settings");
    run(120);
    shot("01_title_dropping");
    run(800);
    shot("02_title");

    screens.handleConfirmPress();
    run(100);
    shot("03_start");
    playPieces(26);
    run(120);
    shot("04_playing");
    held[(uint8_t)Key::DOWN] = true;
    run(120);
    held[(uint8_t)Key::DOWN] = false;
    held[(uint8_t)Key::CONFIRM] = true;
    screens.handleConfirmPress();
    run(460);
    shot("05_pause_bar");
    run(340);
    held[(uint8_t)Key::CONFIRM] = false;
    run(32);
    shot("06_paused");
    screens.handleConfirmPress();
    run(100);
    shot("07_resumed");

    for (uint16_t i = 0; i < 400 && (game.takeEvents() & Game::EVENT_ROWS) == 0; i++)
    {
        waitForPiece();
        if (game.falling())
        {
            for (uint8_t y = ROWS - 1; y > ROWS - 3; y--)
            {
                for (uint8_t x = 1; x < COLS; x++)
                {
                    game.setCell(x, y, (uint8_t)(1 + (x + y) % PIECE_COUNT));
                }
                game.setCell(0, y, 0);
            }
            game.setPiece({0, 1, -1, 8});
            screens.handleUpPress();
            run(16 * 4);
            shot("08_clearing");
            run(600);
            break;
        }
    }
    quit();

    start(Mode::CASCADE);
    setUpCascade();
    run(16);
    shot("09_cascade_set_up");
    screens.handleUpPress();
    run(200);
    shot("10_cascade_wipe");
    for (int i = 0; i < 80 && game.phase() != Game::Phase::SETTLING; i++)
    {
        run(16);
    }
    run(40);
    shot("11_cascade_falling");
    for (int i = 0; i < 300 && !game.falling(); i++)
    {
        run(16);
        if (game.chain() >= 2 && game.phase() == Game::Phase::CLEARING)
        {
            run(50);
            shot("12_cascade_chain");
            break;
        }
    }
    run(600);
    quit();

    start(Mode::SPRINT);
    playPieces(20);
    run(100);
    shot("13_sprint");
    playPieces(200);
    run(300);
    shot("14_sprint_filling");
    run(1600);
    shot("15_sprint_done");
    run(600);
    screens.handleDownPress();
    screens.handleConfirmPress();
    run(1200);
    shot("16_title_after");

    start(Mode::ENDLESS);
    for (uint8_t y = HIDDEN_ROWS + 3; y < ROWS; y++)
    {
        for (uint8_t x = 0; x < COLS; x++)
        {
            game.setCell(x, y, x == y % COLS ? 0 : (uint8_t)(1 + (x * 3 + y) % PIECE_COUNT));
        }
    }
    for (int i = 0; i < 12 && !game.over(); i++)
    {
        screens.handleUpPress();
        run(40);
    }
    run(250);
    shot("17_topping_out");
    run(1400);
    shot("18_topped_out");
    run(600);
    screens.handleDownPress();
    screens.handleConfirmPress();
    run(200);

    screens.setScreen(1);
    run(200);
    shot("19_settings");
    screens.handleUpPress();
    screens.handleConfirmPress();
    run(200);
    shot("20_reset");
    screens.handleBackPress();
    run(200);

    for (uint8_t t = 0; t < THEME_COUNT; t++)
    {
        settings.theme = t;
        applyTheme();
        state = 0xC0FFEEu + t * 977;
        screens.setScreen(1);
        screens.setScreen(0);
        run(1000);
        std::string name = std::string(THEMES[t].name);
        shot("21_title_" + name);
        start(Mode::CASCADE);
        setUpCascade();
        run(32);
        shot("22_play_" + name);
        quit();
    }
    return 0;
}
