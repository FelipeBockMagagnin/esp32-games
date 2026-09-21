#include "CombatScreen.h"

CombatScreen::CombatScreen()
    : Screen(0x0000),
      // Header Navigation
      navMiningArrow(5, 8, 3, 5, image_ButtonLeftSmall_bits, 0xFFFF),
      navMiningText(12, 6, "Mining", 0xFFFF, 1, TL_DATUM, true, 0x0000),
      navSewersArrow(85, 13, 3, 5, image_ButtonLeftSmall_bits, 0xFFFF),
      titleSewers(89, 7, "Sewers", 0xFFFF, 2, TL_DATUM, true, 0x0000),
      navSkillTreeText(172, 5, "Skill Tree", 0xFFFF, 1, TL_DATUM, true, 0x0000),
      navSkillTreeArrow(233, 7, 3, 5, image_ButtonRightSmall_bits, 0xFFFF),

      // Enemy / Central Sprite Section
      enemyRato(58, 39, 128, 128, image_rato_pixels),
      enemyName(93, 171, "Enemy", 0xFFFF, 2, TL_DATUM, true, 0x0000),

      // Enemy Stats Row
      swordIcon(18, 202, 16, 16, image_sword_02b_pixels),
      swordStat(35, 203, "100", 0xFFFF, 2, TL_DATUM, true, 0x0000),
      armorIcon(98, 202, 16, 16, image_armor_01b_pixels),
      armorStat(115, 203, "100", 0xFFFF, 2, TL_DATUM, true, 0x0000),
      enemyHeartIcon(171, 202, 15, 16, image_cards_hearts_bits, 0xC0E5),
      enemyHp(189, 203, "20", 0xFFFF, 2, TL_DATUM, true, 0x0000),

      // Countdown / Progress Section
      timerBox(82, 234, 84, 19, 0xFFFF),
      timerText(112, 240, "3.00s", 0xFFFF, 1, TL_DATUM, false, 0x0000),
      timerMultiplierText(171, 240, "2", 0xE0C4, 1, TL_DATUM, true, 0x0000),

      // Bottom Action / Player Section
      actionArmorIcon(10, 277, 16, 16, image_armor_01b_pixels),
      actionOkBtn1(30, 281, 9, 9, image_Ok_btn_bits, 0x1B9B),
      actionTimer1(30, 278, "9.0s", 0xFFFF, 2, TL_DATUM, true, 0x0000),
      actionSwordIcon(11, 297, 16, 16, image_sword_02b_pixels),
      actionOkBtn2(30, 301, 9, 9, image_Ok_btn_bits, 0xE0C4),
      actionTimer2(31, 298, "3s", 0xFFFF, 2, TL_DATUM, true, 0x0000),
      playerHeartIcon(193, 298, 15, 16, image_cards_hearts_bits, 0xC0E5),
      playerHp(211, 299, "20", 0xFFFF, 2, TL_DATUM, true, 0x0000),

      lastTimerUpdate(0),
      timerCountdown(3.00f)
{
    // Register all elements
    addElement(&enemyRato);
    addElement(&enemyName);

    addElement(&navMiningArrow);
    addElement(&navMiningText);
    addElement(&navSewersArrow);
    addElement(&titleSewers);
    addElement(&navSkillTreeText);
    addElement(&navSkillTreeArrow);

    addElement(&swordIcon);
    addElement(&swordStat);
    addElement(&armorIcon);
    addElement(&armorStat);
    addElement(&enemyHeartIcon);
    addElement(&enemyHp);

    addElement(&timerBox);
    addElement(&timerText);
    addElement(&timerMultiplierText);

    addElement(&actionArmorIcon);
    addElement(&actionOkBtn1);
    addElement(&actionTimer1);
    addElement(&actionSwordIcon);
    addElement(&actionOkBtn2);
    addElement(&actionTimer2);
    addElement(&playerHeartIcon);
    addElement(&playerHp);
}

void CombatScreen::update(unsigned long now)
{
    if (now - lastTimerUpdate >= 100) // Update timer ~10 times per second
    {
        lastTimerUpdate = now;

        timerCountdown -= 0.10f;
        if (timerCountdown <= 0.00f)
        {
            timerCountdown = 3.00f;
        }

        char buf[12];
        dtostrf(timerCountdown, 4, 2, buf);
        String s = String(buf) + "s";
        timerText.setText(s);
    }
}
