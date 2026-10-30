#include "GameScreen.h"
#include <stdio.h>
#include "../ui/PetDraw.h"

static const unsigned long STEP_MS = 400;
static const unsigned long REVEAL_MS = 1400;
static const unsigned long SCORE_MS = 2200;

GameScreen::GameScreen(Pet &pet, LcdMatrix &lcd, Panel &panel, void (*changed)(), ScreenManager &screenManager,
                       SoundManager &sound)
    : pet(pet), lcd(lcd), panel(panel), changed(changed), screenManager(screenManager), sound(sound)
{
}

void GameScreen::onEnter(TFT_eSPI &tft)
{
    phase = Phase::ASK;
    round = 0;
    wins = 0;
    panel.paint(tft);
    panel.setIcons(tft, ICON_GAME, pet.attention());
    draw();
    lcd.present(tft);
}

void GameScreen::draw()
{
    const Sprite &art = FORM_ART[pet.form()];
    int16_t x = (LcdMatrix::COLS - art.w) / 2;
    char line[6];
    Pose pose;
    pose.second = second;
    lcd.clear();
    switch (phase)
    {
    case Phase::ASK:
        lcd.spriteOn(art, x, LcdMatrix::ROWS, false, pose);
        lcd.text(1, 5, "?");
        lcd.text(LcdMatrix::COLS - 4, 5, "?");
        break;
    case Phase::REVEAL:
        pose.cheer = won;
        pose.mouthOpen = won;
        lcd.spriteOn(art, turnedLeft ? 0 : LcdMatrix::COLS - art.w, LcdMatrix::ROWS, !turnedLeft, pose);
        lcd.text(turnedLeft ? LcdMatrix::COLS - 13 : 1, 5, won ? "YES" : "NO");
        break;
    case Phase::SCORE:
        snprintf(line, sizeof(line), "%u/%u", wins, GAME_ROUNDS);
        lcd.textCentered(1, line);
        lcd.textCentered(9, wins >= GAME_WINS_NEEDED ? "WIN" : "LOSE");
        break;
    }
}

void GameScreen::update(unsigned long now)
{
    if (pet.isDead() || pet.isSleeping())
    {
        screenManager.closeModal();
        return;
    }
    if (phase != Phase::ASK && (long)(now - phaseEndsAt) >= 0)
    {
        if (phase == Phase::SCORE)
        {
            finish();
            return;
        }
        if (round >= GAME_ROUNDS)
        {
            phase = Phase::SCORE;
            phaseEndsAt = now + SCORE_MS;
            if (wins >= GAME_WINS_NEEDED)
            {
                sound.playWin();
            }
            else
            {
                sound.playLose();
            }
        }
        else
        {
            phase = Phase::ASK;
        }
        draw();
    }
    if ((long)(now - nextStepAt) >= 0)
    {
        nextStepAt = now + STEP_MS;
        second = !second;
        draw();
    }
}

void GameScreen::render(TFT_eSPI &tft)
{
    panel.setIcons(tft, ICON_GAME, pet.attention());
    lcd.present(tft);
}

void GameScreen::guess(bool left)
{
    if (phase != Phase::ASK)
    {
        return;
    }
    turnedLeft = pet.rollGameDirection();
    won = left == turnedLeft;
    wins += won ? 1 : 0;
    round++;
    phase = Phase::REVEAL;
    phaseEndsAt = millis() + REVEAL_MS;
    if (won)
    {
        sound.playCare();
    }
    else
    {
        sound.playLose();
    }
    draw();
}

void GameScreen::finish()
{
    pet.finishGame(wins);
    changed();
    screenManager.closeModal();
}

bool GameScreen::onBackPress()
{
    guess(true);
    return true;
}

bool GameScreen::onRightPress()
{
    guess(false);
    return true;
}

void GameScreen::onUpPress()
{
    sound.playMenu();
    screenManager.closeModal();
}

void GameScreen::onConfirmPress()
{
    if (phase == Phase::SCORE)
    {
        finish();
    }
}
