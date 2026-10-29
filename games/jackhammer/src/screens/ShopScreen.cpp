#include "ShopScreen.h"
#include "../assets/Assets.h"
#include "../render/StyleArt.h"

static const int16_t ROW_X = 5;
static const int16_t FIRST_ROW_Y = 36;
static const int16_t ROW_PITCH = ListRow::DEFAULT_H + 6;
static const int16_t COIN_Y = 248;
static const int16_t HINT_Y = 300;

ShopScreen::ShopScreen(Profile &profile, ScreenManager &screenManager, SoundManager &sound, void (*changed)())
    : header("Shop"),
      coinText(120, COIN_Y, "", Theme::GOLD, 2, TC_DATUM),
      hintText(120, HINT_Y, "Confirm: buy or use - Right: shelf", Theme::TEXT_DIM, 1, TC_DATUM),
      profile(profile),
      screenManager(screenManager),
      sound(sound),
      changed(changed)
{
    addElement(&header);
    setHeader(&header);
    for (uint8_t i = 0; i < MAX_ROWS; i++)
    {
        rows[i].setPosition(ROW_X, FIRST_ROW_Y + i * ROW_PITCH);
        addElement(&rows[i]);
    }
    addElement(&coinText);
    addElement(&hintText);
}

void ShopScreen::build()
{
    bool styles = shelf == Shelf::STYLES;
    header.setTitle(styles ? "Styles" : "Music");
    header.setBackLabel("Back");
    header.setNextLabel(styles ? "Music" : "Styles");
    for (uint8_t i = 0; i < MAX_ROWS; i++)
    {
        rows[i].setVisible(i < shelfSize(shelf));
        if (i >= shelfSize(shelf))
        {
            continue;
        }
        const ShopItem &item = shopItem(shelf, i);
        rows[i].setTitle(item.name);
        rows[i].setSubtitle(item.note);
        if (styles)
        {
            rows[i].setIconPixels(STYLE_ART[i].icon, 16, 16);
        }
        else
        {
            rows[i].setIconBitmap(image_Note_bits, 8, 12, Theme::INFO);
        }
    }
}

void ShopScreen::refreshRows()
{
    char buf[16];
    for (uint8_t i = 0; i < shelfSize(shelf); i++)
    {
        ListRow &row = rows[i];
        bool owned = owns(profile, shelf, i);
        bool affordable = profile.coins >= shopItem(shelf, i).price;
        if (isEquipped(profile, shelf, i))
        {
            row.setValue(shelf == Shelf::STYLES ? "Worn" : "Playing");
            row.setValueColor(Theme::GOOD);
            row.setShowCoin(false);
        }
        else if (owned)
        {
            row.setValue("Owned");
            row.setValueColor(Theme::TEXT_2);
            row.setShowCoin(false);
        }
        else
        {
            snprintf(buf, sizeof(buf), "%u", (unsigned)shopItem(shelf, i).price);
            row.setValue(buf);
            row.setValueColor(affordable ? Theme::GOLD : Theme::BAD);
            row.setShowCoin(true);
        }
        ListRow::State state = owned ? ListRow::OWNED : (affordable ? ListRow::NORMAL : ListRow::DIMMED);
        row.setState(i == selected ? ListRow::SELECTED : state);
    }
    snprintf(buf, sizeof(buf), "%lu coins", (unsigned long)profile.coins);
    coinText.setText(buf);
}

void ShopScreen::onEnter(TFT_eSPI &tft)
{
    selected = shelf == Shelf::STYLES ? profile.style : profile.song;
    build();
    refreshRows();
    Screen::onEnter(tft);
}

void ShopScreen::onUpPress()
{
    selected = selected > 0 ? selected - 1 : shelfSize(shelf) - 1;
    refreshRows();
    sound.playMenu();
}

void ShopScreen::onDownPress()
{
    selected = (selected + 1) % shelfSize(shelf);
    refreshRows();
    sound.playMenu();
}

void ShopScreen::onConfirmPress()
{
    bool bought = !owns(profile, shelf, selected);
    if (!buyOrEquip(profile, shelf, selected))
    {
        sound.playError();
        return;
    }
    changed();
    refreshRows();
    if (bought)
    {
        sound.playBuy();
    }
    else
    {
        sound.playMenu();
    }
}

bool ShopScreen::onBackPress()
{
    sound.playMenu();
    screenManager.closeModal();
    return true;
}

bool ShopScreen::onRightPress()
{
    shelf = shelf == Shelf::STYLES ? Shelf::SONGS : Shelf::STYLES;
    selected = shelf == Shelf::STYLES ? profile.style : profile.song;
    build();
    refreshRows();
    sound.playMenu();
    return true;
}
