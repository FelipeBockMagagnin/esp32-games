#include "InventoryScreen.h"
#include "../nan/Format.h"
#include "../ui/Art.h"
#include "../ui/Labels.h"

static const int16_t ROW_Y = 24;
static const int16_t ROW_H = 32;

static const char *const PLACE_NAMES[EQUIP_COUNT] = {"Cabeca", "Corpo", "Pernas", "Pes", "Arma", "Acessorio 1",
                                                     "Acessorio 2"};

InventoryScreen::InventoryScreen(Game &game, SoundManager &sound)
    : Screen("Itens"),
      game(game),
      sound(sound),
      total(label(DISPLAY_W / 2, 255, TC_DATUM)),
      hint(label(DISPLAY_W / 2, 270, TC_DATUM, Theme::TEXT_DIM))
{
    for (uint8_t i = 0; i < ROWS; i++)
    {
        ItemRow &row = rows[i];
        int16_t y = ROW_Y + i * ROW_H;
        row.frame = FocusFrame(5, y, 230, ROW_H - 2);
        row.icon = Icon16(11, y + 7);
        row.caption = label(34, y + 4, TL_DATUM, Theme::TEXT_DIM);
        row.name = label(34, y + 16);
        row.attack = label(229, y + 4, TR_DATUM, Theme::BAD);
        row.defense = label(229, y + 16, TR_DATUM, Theme::SELECT);
        addElement(&row.icon);
        addElement(&row.caption);
        addElement(&row.name);
        addElement(&row.attack);
        addElement(&row.defense);
        addElement(&row.frame);
    }
    addElement(&total);
    addElement(&hint);
    addTaskbar();
}

void InventoryScreen::onEnter(TFT_eSPI &tft)
{
    picking = false;
    forceRefresh();
    refresh();
    Screen::onEnter(tft);
}

void InventoryScreen::update(unsigned long now)
{
    // Drops land while the screen is open
    if (refreshDue(now))
    {
        refresh();
    }
}

void InventoryScreen::fillRow(ItemRow &row, const char *caption, int8_t item, bool selected)
{
    char buf[24];
    for (UI *e : {(UI *)&row.frame, (UI *)&row.icon, (UI *)&row.caption, (UI *)&row.name, (UI *)&row.attack,
                  (UI *)&row.defense})
    {
        e->setVisible(true);
    }
    row.frame.setOn(selected);
    row.caption.setText(caption);
    if (item < 0)
    {
        row.icon.setPixels(nullptr);
        row.name.setText("(vazio)");
        row.name.setColor(Theme::TEXT_DIM);
        row.attack.setText("");
        row.defense.setText("");
        return;
    }
    row.icon.setPixels(iconFor(ITEMS[item].art));
    snprintf(buf, sizeof(buf), "%s Nv %u", ITEMS[item].name, (unsigned)game.itemLevel(item));
    row.name.setText(buf);
    row.name.setColor(Theme::TEXT);
    double atk = game.itemPercent(item, true);
    double def = game.itemPercent(item, false);
    char n[12];
    formatNumber(atk, n, sizeof(n));
    snprintf(buf, sizeof(buf), "+%s%% ATQ", n);
    row.attack.setText(atk > 0 ? buf : "");
    formatNumber(def, n, sizeof(n));
    snprintf(buf, sizeof(buf), "+%s%% DEF", n);
    row.defense.setText(def > 0 ? buf : "");
}

void InventoryScreen::refresh()
{
    char a[12], b[12], buf[64];
    if (picking)
    {
        fillRow(rows[0], "Tirar", -1, choice == 0);
        for (uint8_t i = 0; i < choiceCount; i++)
        {
            bool worn = false;
            for (uint8_t p = 0; p < EQUIP_COUNT; p++)
            {
                worn |= game.equippedAt(p) == choices[i];
            }
            fillRow(rows[i + 1], worn ? "Em uso" : "Na mochila", choices[i], choice == i + 1);
        }
        for (uint8_t i = choiceCount + 1; i < ROWS; i++)
        {
            for (UI *e : {(UI *)&rows[i].frame, (UI *)&rows[i].icon, (UI *)&rows[i].caption, (UI *)&rows[i].name,
                          (UI *)&rows[i].attack, (UI *)&rows[i].defense})
            {
                e->setVisible(false);
            }
        }
        snprintf(buf, sizeof(buf), "Escolha: %s", PLACE_NAMES[place]);
        total.setText(buf);
        hint.setText("Confirm veste, Esquerda volta");
        return;
    }

    for (uint8_t i = 0; i < ROWS; i++)
    {
        fillRow(rows[i], PLACE_NAMES[i], game.equippedAt(i), i == place);
    }
    formatNumber(game.gearPercent(true), a, sizeof(a));
    formatNumber(game.gearPercent(false), b, sizeof(b));
    snprintf(buf, sizeof(buf), "Total +%s%% ATQ  +%s%% DEF", a, b);
    total.setText(buf);
    hint.setText("Repetidos sobem o nivel");
}

void InventoryScreen::openPicker()
{
    choiceCount = 0;
    Slot slot = Game::placeSlot(place);
    for (uint8_t i = 0; i < ITEM_COUNT && choiceCount < ROWS - 1; i++)
    {
        if (ITEMS[i].slot == slot && game.itemLevel(i) > 0)
        {
            choices[choiceCount++] = i;
        }
    }
    picking = true;
    choice = 0;
    for (uint8_t i = 0; i < choiceCount; i++)
    {
        if (choices[i] == game.equippedAt(place))
        {
            choice = i + 1;
        }
    }
}

void InventoryScreen::onUpPress()
{
    uint8_t count = picking ? choiceCount + 1 : ROWS;
    uint8_t &cursor = picking ? choice : place;
    cursor = (cursor + count - 1) % count;
    sound.playMenu();
    refresh();
}

void InventoryScreen::onDownPress()
{
    uint8_t count = picking ? choiceCount + 1 : ROWS;
    uint8_t &cursor = picking ? choice : place;
    cursor = (cursor + 1) % count;
    sound.playMenu();
    refresh();
}

bool InventoryScreen::onRepeat(Key key)
{
    if (key == Key::UP || key == Key::DOWN)
    {
        key == Key::UP ? onUpPress() : onDownPress();
        return true;
    }
    return false;
}

void InventoryScreen::onConfirmPress()
{
    if (!picking)
    {
        openPicker();
        sound.playMenu();
        refresh();
        return;
    }
    game.equip(place, choice == 0 ? -1 : choices[choice - 1]);
    picking = false;
    sound.playBuy();
    refresh();
}

bool InventoryScreen::onBackPress()
{
    if (!picking)
    {
        return false;
    }
    picking = false;
    sound.playMenu();
    refresh();
    return true;
}
