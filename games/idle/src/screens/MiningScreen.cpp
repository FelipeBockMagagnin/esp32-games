#include "MiningScreen.h"
#include "../assets/Sprites.h"
#include "../game/Format.h"
#include "../game/AscensionConfig.h"


// The colors image_Icon31_33_pixels is drawn in (light, mid, dark), which each ore tier's
// palette replaces. Must stay in step with the asset; copper's row in ORE_TIERS repeats them.
static const uint16_t ORE_SPRITE_PALETTE[3] = {0xFC08, 0xD163, 0x806A};
static const uint8_t ORE_PALETTE_SIZE = 3;

static const uint16_t CARD_BORDER = Theme::GOLD_FRAME; // Dark amber, the gold card's frame
static const uint16_t RATE_COLOR = Theme::GOOD;  // Soft green: income that grows on its own
static const uint16_t HINT_COLOR = Theme::TEXT_2;  // Mid gray for secondary lines
static const uint16_t XP_BAR_COLOR = Theme::XP_BAR; // Teal fill, with a near-black tint and a mid tone
static const uint16_t XP_BAR_BG = Theme::XP_TRACK;    // of the same hue, like the combat bars
static const uint16_t XP_BAR_BORDER = Theme::XP_BORDER;
static const uint16_t PROMPT_COLOR = Theme::CONFIRM; // The Confirm cap's red, so the prompt points at it
static const uint16_t GOAL_COLOR = Theme::GOLD_BAR;   // Amber fill, dark enough for the white label,
static const uint16_t GOAL_BG = Theme::GOLD_TRACK;      // the same progress style as the Awards screen
static const uint16_t GOAL_BORDER = Theme::GOLD_BORDER;

// Layout coordinates
static const int16_t CENTER_X = 120;
static const int16_t MARGIN_X = 12;
static const int16_t RIGHT_X = 240 - MARGIN_X;

// Gold card
static const int16_t CARD_X = 8;
static const int16_t CARD_Y = 32;
static const int16_t CARD_W = 224;
static const int16_t CARD_H = 46;
static const int16_t COIN_RADIUS = 8;
static const int16_t COIN_PULSE_RADIUS = 10;
static const unsigned long COIN_PULSE_MS = 120;
static const unsigned long LEVEL_FLASH_MS = 400;
static const int16_t COIN_X = CARD_X + 18;
static const int16_t COIN_Y = CARD_Y + 18;
static const int16_t GOLD_TEXT_X = CARD_X + 34;
static const int16_t GOLD_AMOUNT_Y = CARD_Y + 6; // Size 3: 24px tall
static const int16_t GOLD_RATE_Y = CARD_Y + 32;

// Ore display: the sprite covers the space above the ore so popups can float into it
static const int16_t ORE_AREA_X = 32;
static const int16_t ORE_AREA_Y = 80;
static const int16_t ORE_AREA_W = 176;
static const int16_t ORE_AREA_H = 118;
static const int16_t ORE_SIZE = 96;
static const int16_t ORE_NAME_Y = 198;
static const int16_t NEXT_ORE_Y = 219;

// Level row: label and XP count above a full-width bar
static const int16_t LEVEL_Y = 244;
static const int16_t XP_TEXT_Y = 250; // Bottom-aligned with the size-2 level label
static const int16_t XP_BAR_Y = 262;
static const int16_t XP_BAR_H = 12;

// Confirm hint; text (8px) and badge (11px) are both centred on the row's middle
static const int16_t MINE_ROW_Y = 288;
static const int16_t MINE_LABEL_X = MARGIN_X + ButtonBadge::SIZE + 4;
static const int16_t BADGE_OFFSET_Y = (8 - ButtonBadge::SIZE) / 2;

// Goal bar along the bottom edge
static const int16_t GOAL_Y = 302;
static const int16_t GOAL_H = 15;

// A vein blinks for this long before it goes, and what one gave stays on screen this long
static const unsigned long VEIN_FADE_MS = 3000;
static const unsigned long VEIN_MESSAGE_MS = 4000;

// First-click prompt: shown until the player has mined this many times, blinking
static const uint32_t PROMPT_CLICKS = 3;
static const unsigned long PROMPT_BLINK_MS = 500;

static const uint8_t MINE_CHIPS = 4;
static const uint8_t LEVEL_UP_SPARKS = 14;
static const uint8_t LUCKY_SPARKS = 10;

MiningScreen::MiningScreen(GameState &game, const Unlocks &unlocks, SoundManager &sound)
    : header("Mining"),

      // Gold card
      goldCard(CARD_X, CARD_Y, CARD_W, CARD_H, CARD_BORDER, true, Theme::CARD, true, 4),
      goldCoin(COIN_X, COIN_Y, COIN_RADIUS, COIN_RADIUS, Theme::GOLD, true),
      goldLabel(GOLD_TEXT_X, GOLD_AMOUNT_Y, "0", Theme::GOLD, 3),
      goldRate(GOLD_TEXT_X, GOLD_RATE_Y, "+0 gold/s", RATE_COLOR, 1),

      // Center ore display; popups are gold to match the balance they add to
      oreDisplay(ORE_AREA_X, ORE_AREA_Y, ORE_AREA_W, ORE_AREA_H, image_Icon31_33_pixels,
                 (ORE_AREA_W - ORE_SIZE) / 2, 14, ORE_SIZE, ORE_SIZE, Theme::GOLD),
      // Centered: the ore name's length changes with the tier ("Gold Ore" vs "Mythril Ore")
      currentOreText(CENTER_X, ORE_NAME_Y, "", Theme::TEXT, 2, TC_DATUM),
      nextOreText(CENTER_X, NEXT_ORE_Y, "", HINT_COLOR, 1, TC_DATUM),

      // Level row
      levelText(MARGIN_X, LEVEL_Y, "Level 1", Theme::TEXT, 2),
      xpText(RIGHT_X, XP_TEXT_Y, "", HINT_COLOR, 1, TR_DATUM),
      expBar(MARGIN_X, XP_BAR_Y, RIGHT_X - MARGIN_X, XP_BAR_H, 0, XP_PER_LEVEL, XP_BAR_COLOR, XP_BAR_BG, XP_BAR_BORDER),

      // Confirm hint
      mineBadge(MARGIN_X, MINE_ROW_Y + BADGE_OFFSET_Y, ButtonBadge::CONFIRM),
      mineLabel(MINE_LABEL_X, MINE_ROW_Y, "Mine", Theme::TEXT, 1),
      mineValue(RIGHT_X, MINE_ROW_Y, "", Theme::GOLD, 1, TR_DATUM),

      goalBar(MARGIN_X, GOAL_Y, RIGHT_X - MARGIN_X, GOAL_H, 0, 1, GOAL_COLOR, GOAL_BG, GOAL_BORDER),

      game(game),
      unlocks(unlocks),
      sound(sound),
      shownTier(0),
      prompting(false),
      seenVeinSpawns(0),
      veinMessageUntil(0),
      veinMessage(""),
      veinMessageColor(Theme::GOLD),
      coinPulseUntil(0),
      levelFlashUntil(0)
{
    addElement(&header);
    setHeader(&header);

    // Gold card
    addElement(&goldCard);
    addElement(&goldCoin, Theme::CARD);
    addElement(&goldLabel, Theme::CARD);
    addElement(&goldRate, Theme::CARD);

    // Center ore display
    addElement(&oreDisplay);
    addElement(&currentOreText);
    addElement(&nextOreText);

    // Level row
    addElement(&levelText);
    addElement(&xpText);
    addElement(&expBar);

    // Confirm hint
    addElement(&mineBadge);
    addElement(&mineLabel);
    addElement(&mineValue);

    addElement(&goalBar);

    applyOreTier(0);
}

// Everything that only changes when the ore does: its colors, its name and the hint
// pointing at the next one. Kept out of the 10 Hz refresh, which only calls it on a change.
void MiningScreen::applyOreTier(uint8_t tier)
{
    shownTier = tier;
    const OreTierDef &ore = ORE_TIERS[tier];

    oreDisplay.setPalette(ORE_SPRITE_PALETTE, ore.palette, ORE_PALETTE_SIZE);
    oreDisplay.setGlowColor(ore.palette[0]);
    currentOreText.setText(ore.name);
    currentOreText.setColor(ore.palette[0]);

    if (tier + 1 < ORE_TIER_COUNT)
    {
        char buf[40];
        snprintf(buf, sizeof(buf), "Next: %s at Lv %u", ORE_TIERS[tier + 1].name,
                 (unsigned)ORE_TIERS[tier + 1].minLevel);
        nextOreText.setText(buf);
    }
    else
    {
        nextOreText.setText("Richest ore reached");
    }
}

void MiningScreen::onEnter(TFT_eSPI &tft)
{
    // A vein that appeared while the player was elsewhere is shown without the chime
    seenVeinSpawns = game.getVeinSpawns();
    Screen::onEnter(tft);
}

// The vein beside the ore follows the game's state every frame (it can appear or be missed
// at any moment); the side alternates from one vein to the next
void MiningScreen::refreshVein(unsigned long now)
{
    if (game.getVeinSpawns() != seenVeinSpawns)
    {
        seenVeinSpawns = game.getVeinSpawns();
        sound.playUnlock();
    }
    if (game.isVeinVisible())
    {
        oreDisplay.setVein(image_icon_gold_vein_pixels, seenVeinSpawns % 2 == 0,
                           game.getVeinRemaining(now) < VEIN_FADE_MS);
    }
    else
    {
        oreDisplay.clearVein();
    }
}

void MiningScreen::update(unsigned long now)
{
    // Animations run at their own frame rate, independent of the text refresh below
    refreshVein(now);
    oreDisplay.update(now);
    goldCoin.setRadius((long)(now - coinPulseUntil) < 0 ? COIN_PULSE_RADIUS : COIN_RADIUS);

    if (!refreshDue(now))
    {
        return;
    }

    goldLabel.setText(formatAmount(game.getGold()));

    // A running vein bonus takes the rate line, with its multiplier and the time left
    char buf[40];
    unsigned long buffLeft = game.getBuffRemaining(now);
    VeinEffect buff = game.getActiveBuff();
    if (buff == VeinEffect::FRENZY)
    {
        snprintf(buf, sizeof(buf), "%s  FRENZY x%u %lus", formatRate(game.getProductionPerSecond()).c_str(),
                 (unsigned)VEIN_FRENZY_MULTIPLIER, (buffLeft + 999) / 1000);
        goldRate.setText(buf);
        goldRate.setColor(Theme::GOLD);
    }
    else if (buff == VeinEffect::CLICK_FRENZY)
    {
        snprintf(buf, sizeof(buf), "%s  CLICKS x%u %lus", formatRate(game.getProductionPerSecond()).c_str(),
                 (unsigned)VEIN_CLICK_MULTIPLIER, (buffLeft + 999) / 1000);
        goldRate.setText(buf);
        goldRate.setColor(Theme::GOLD);
    }
    else
    {
        goldRate.setText(formatRate(game.getProductionPerSecond()));
        goldRate.setColor(RATE_COLOR);
    }

    // The line under the ore name carries, most urgent first: a vein waiting to be caught,
    // what the last one gave, the first-click prompt for a brand-new player, and otherwise
    // the next ore
    uint8_t tier = game.getOreTier();
    bool prompt = game.getTotalClicks() < PROMPT_CLICKS;
    if (game.isVeinVisible())
    {
        prompting = true;
        nextOreText.setText("GOLD VEIN! Press Up to catch it");
        nextOreText.setColor((now / PROMPT_BLINK_MS) % 2 == 0 ? Theme::GOLD : Theme::TEXT);
    }
    else if ((long)(now - veinMessageUntil) < 0)
    {
        prompting = true;
        nextOreText.setText(veinMessage);
        nextOreText.setColor(veinMessageColor);
    }
    else if (prompt)
    {
        prompting = true;
        nextOreText.setText("Press the red button to mine!");
        nextOreText.setColor((now / PROMPT_BLINK_MS) % 2 == 0 ? PROMPT_COLOR : Theme::TEXT);
    }
    else if (prompting || tier != shownTier)
    {
        prompting = false;
        nextOreText.setColor(HINT_COLOR);
        applyOreTier(tier);
    }

    const Unlocks::Goal *goal = unlocks.getGoal();
    goalBar.setVisible(goal != nullptr);
    if (goal != nullptr)
    {
        goalBar.setLabel(goal->label);
        goalBar.setProgress((int32_t)min(goal->done, goal->target), (int32_t)max(goal->target, (uint32_t)1));
    }

    snprintf(buf, sizeof(buf), "Level %u", (unsigned)game.getMiningLevel());
    levelText.setText(buf);

    bool levelFlash = (long)(now - levelFlashUntil) < 0;
    levelText.setColor(levelFlash ? Theme::INFO : Theme::TEXT);
    expBar.setFillColor(levelFlash ? Theme::INFO : XP_BAR_COLOR);

    uint32_t xp = game.getMiningXp();
    uint32_t xpToNext = game.getXpToNextLevel();
    expBar.setProgress(xp, xpToNext);
    snprintf(buf, sizeof(buf), "%lu / %lu XP", (unsigned long)xp, (unsigned long)xpToNext);
    xpText.setText(buf);

    snprintf(buf, sizeof(buf), "+%s gold  +%lu XP", formatAmount(game.getClickAmount()).c_str(),
             (unsigned long)game.getXpPerClick());
    mineValue.setText(buf);
    mineValue.setColor(buff == VeinEffect::CLICK_FRENZY ? Theme::SELECT : Theme::GOLD);

    // The running click combo, worth keeping up
    uint32_t combo = game.getComboPercent(now);
    if (combo > 0)
    {
        snprintf(buf, sizeof(buf), "Mine x%lu.%02lu", (unsigned long)(1 + combo / 100), (unsigned long)(combo % 100));
        mineLabel.setText(buf);
        mineLabel.setColor(Theme::SELECT);
    }
    else
    {
        mineLabel.setText("Mine");
        mineLabel.setColor(Theme::TEXT);
    }
}

void MiningScreen::pulseCoin()
{
    coinPulseUntil = millis() + COIN_PULSE_MS;
}

void MiningScreen::onConfirmPress()
{
    bool leveledUp;
    unsigned long now = millis();
    uint64_t gained = game.mine(leveledUp, now);

    oreDisplay.shake();
    oreDisplay.burst(MINE_CHIPS, false);
    pulseCoin();

    // Both would rise from the same spot, so a level-up takes the popup
    if (leveledUp)
    {
        oreDisplay.addPopup("LEVEL UP!", Theme::INFO);
        levelFlashUntil = now + LEVEL_FLASH_MS;
        oreDisplay.burst(LEVEL_UP_SPARKS, true);
        sound.playLevelUp();
    }
    else
    {
        if (game.wasLastMineDoubled())
        {
            oreDisplay.addPopup(String("x2 +") + formatAmount(gained), Theme::MYTHIC);
        }
        else
        {
            oreDisplay.addPopup(String("+") + formatAmount(gained));
        }
        sound.playMine((uint8_t)(game.getComboPercent(now) / COMBO_STEP_PERCENT));
    }
    forceRefresh();
}

// Catches the vein beside the ore, if one is there, and says what it gave on the line under
// the ore name for a few seconds; a Lucky also floats its gold up from the ore
void MiningScreen::onUpPress()
{
    unsigned long now = millis();
    VeinEffect effect = game.catchVein(now);
    if (effect == VeinEffect::NONE)
    {
        return;
    }

    sound.playDrop();
    oreDisplay.clearVein();
    char buf[40];
    switch (effect)
    {
    case VeinEffect::LUCKY:
        snprintf(buf, sizeof(buf), "LUCKY! +%s gold", formatAmount(game.getLastLuckyGold()).c_str());
        oreDisplay.addPopup(String("+") + formatAmount(game.getLastLuckyGold()));
        oreDisplay.burst(LUCKY_SPARKS, true);
        pulseCoin();
        break;
    case VeinEffect::FRENZY:
        snprintf(buf, sizeof(buf), "FRENZY! Gold/s x%u for %lus", (unsigned)VEIN_FRENZY_MULTIPLIER,
                 (game.getBuffRemaining(now) + 999) / 1000);
        break;
    default:
        snprintf(buf, sizeof(buf), "CLICK FRENZY! x%u for %lus", (unsigned)VEIN_CLICK_MULTIPLIER,
                 (game.getBuffRemaining(now) + 999) / 1000);
        break;
    }
    veinMessage = buf;
    veinMessageColor = Theme::GOLD;
    veinMessageUntil = now + VEIN_MESSAGE_MS;
    forceRefresh();
}
