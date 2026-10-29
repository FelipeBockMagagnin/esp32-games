#include "HabitatScreen.h"
#include "../game/Format.h"

static const int16_t STATUS_Y = Header::HEIGHT + GRID_ROWS * TILE_SIZE;
static const int16_t LINE_1_Y = STATUS_Y + 5;
static const int16_t LINE_2_Y = STATUS_Y + 21;

HabitatScreen::HabitatScreen(World &world, Colony &colony, SoundManager &sound, BuildMenuScreen &menu)
    : header("Habitat"),
      grid(0, Header::HEIGHT, world, colony),
      divider(0, STATUS_Y, 240, 1, Theme::FRAME),
      tileText(4, LINE_1_Y, "", Theme::TEXT),
      hintText(236, LINE_1_Y, "", Theme::TEXT_2, 1, TR_DATUM),
      coinsText(4, LINE_2_Y, "", Theme::GOLD),
      foodText(120, LINE_2_Y, "", Theme::GOOD, 1, TC_DATUM),
      popText(236, LINE_2_Y, "", Theme::TEXT_2, 1, TR_DATUM),
      world(world),
      colony(colony),
      sound(sound),
      menu(menu),
      cursorX(START_LEFT + START_SIZE / 2),
      cursorY(START_TOP + START_SIZE / 2)
{
    addElement(&header);
    setHeader(&header);
    addElement(&grid);
    addElement(&divider);
    addElement(&tileText);
    addElement(&hintText);
    addElement(&coinsText);
    addElement(&foodText);
    addElement(&popText);
    grid.setCursor(cursorX, cursorY);
}

void HabitatScreen::update(unsigned long now)
{
    grid.update(now);
    if (!refreshDue(now))
    {
        return;
    }
    refreshStatus();
}

bool HabitatScreen::move(int8_t dx, int8_t dy)
{
    int16_t nx = cursorX + dx, ny = cursorY + dy;
    if (!World::inGrid(nx, ny))
    {
        return false;
    }
    cursorX = nx;
    cursorY = ny;
    grid.setCursor(cursorX, cursorY);
    sound.playStep();
    forceRefresh();
    return true;
}

bool HabitatScreen::onArrowRepeat(Arrow arrow)
{
    static const int8_t DX[] = {0, 0, -1, 1}, DY[] = {-1, 1, 0, 0};
    return move(DX[(uint8_t)arrow], DY[(uint8_t)arrow]);
}

void HabitatScreen::onConfirmPress()
{
    uint16_t i = World::indexOf(cursorX, cursorY);
    forceRefresh();
    switch (world.kindAt(i))
    {
    case TileKind::WILD:
        if (world.clear(i))
        {
            sound.playBuy();
        }
        else
        {
            sound.playError();
        }
        return;
    case TileKind::CROP:
        if (world.isRipe(i))
        {
            if (world.harvest(i))
            {
                sound.playHarvest();
            }
            else
            {
                sound.playError();
            }
            return;
        }
        if (!world.isWatered(i) && world.water(i))
        {
            sound.playWater();
            return;
        }
        break;
    default:
        break;
    }
    sound.playMenu();
    menu.openFor(i);
}

void HabitatScreen::describeTile(char *line, size_t lineSize, char *hint, size_t hintSize, Color &color)
{
    color = Theme::TEXT;
    hint[0] = '\0';
    for (uint8_t c = 0; c < colony.getCount(); c++)
    {
        const Critter &critter = colony.get(c);
        if (critter.x == cursorX && critter.y == cursorY)
        {
            snprintf(line, lineSize, "%s: %s", CRITTER_NAMES[critter.name], describeCritter(critter));
            color = critter.hunger >= NEED_MAX ? Theme::BAD : Theme::INFO;
            return;
        }
    }

    uint16_t i = World::indexOf(cursorX, cursorY);
    switch (world.kindAt(i))
    {
    case TileKind::WILD:
        if (world.canClear(i))
        {
            snprintf(line, lineSize, "Wild");
            snprintf(hint, hintSize, "OK: clear %s", formatAmount(world.clearPrice()).c_str());
            color = world.getCoins() >= world.clearPrice() ? Theme::TEXT : Theme::TEXT_DIM;
        }
        else
        {
            snprintf(line, lineSize, "Wild, too deep to reach");
            color = Theme::TEXT_DIM;
        }
        return;
    case TileKind::GRASS:
        snprintf(line, lineSize, "Grass");
        snprintf(hint, hintSize, "OK: build");
        return;
    case TileKind::SOIL:
        snprintf(line, lineSize, "Soil");
        snprintf(hint, hintSize, "OK: plant");
        return;
    case TileKind::CROP:
    {
        const CropDef &crop = CROPS[world.cropAt(i)];
        if (world.isRipe(i))
        {
            uint32_t gain = world.harvestFood(world.cropAt(i));
            bool room = world.getFood() + gain <= world.getPantrySize();
            snprintf(line, lineSize, "%s, ripe", crop.name);
            snprintf(hint, hintSize, room ? "OK: harvest +%u" : "Pantry full", (unsigned)gain);
            color = room ? Theme::GOOD : Theme::BAD;
        }
        else if (world.isWatered(i))
        {
            snprintf(line, lineSize, "%s %u%%, watered", crop.name, (unsigned)world.growthPercent(i));
            snprintf(hint, hintSize, "OK: options");
        }
        else
        {
            snprintf(line, lineSize, "%s %u%%, dry", crop.name, (unsigned)world.growthPercent(i));
            snprintf(hint, hintSize, "OK: water");
            color = Theme::GOLD;
        }
        return;
    }
    case TileKind::BURROW:
        snprintf(line, lineSize, "Burrow");
        break;
    case TileKind::TROUGH:
        snprintf(line, lineSize, "Trough");
        break;
    case TileKind::POND:
        snprintf(line, lineSize, "Pond");
        break;
    case TileKind::FLOWERS:
        snprintf(line, lineSize, "Flowers");
        break;
    default:
        line[0] = '\0';
        break;
    }
    snprintf(hint, hintSize, "OK: options");
}

void HabitatScreen::refreshStatus()
{
    char line[32];
    char hint[24];
    Color color;
    describeTile(line, sizeof(line), hint, sizeof(hint), color);
    tileText.setText(line);
    tileText.setColor(color);
    hintText.setText(hint);

    char buf[24];
    snprintf(buf, sizeof(buf), "%s coins", formatAmount(world.getCoins()).c_str());
    coinsText.setText(buf);
    snprintf(buf, sizeof(buf), "Food %u/%u", (unsigned)world.getFood(), (unsigned)world.getPantrySize());
    foodText.setText(buf);
    foodText.setColor(world.getFood() == 0 ? Theme::BAD : Theme::GOOD);
    snprintf(buf, sizeof(buf), "Pop %u/%u", (unsigned)colony.getCount(), (unsigned)colony.getCapacity());
    popText.setText(buf);
    popText.setColor(colony.isHoused() ? Theme::TEXT_2 : Theme::BAD);
}
