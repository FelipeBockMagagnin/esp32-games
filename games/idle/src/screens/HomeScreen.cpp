#include "HomeScreen.h"

HomeScreen::HomeScreen()
    : Screen(0x0000),
      // Header
      titleSewers(89, 6, "Sewers", 0xFFFF, 2, TL_DATUM, true, 0x0000),
      navInventory(177, 5, "Inventory", 0xFFFF, 1, TL_DATUM, true, 0x0000),
      navArrow(233, 6, 3, 5, image_ButtonRightSmall_bits, 0xFFFF),

      // Enemy Section
      enemyFrame(58, 34, 130, 151, 0xFFFF),
      enemyRato(59, 35, 128, 128, image_rato_pixels),
      enemyName(94, 167, "Enemy", 0xFFFF, 2, TL_DATUM, true, 0x0000),

      // Enemy Stats Bar
      statsBar(11, 184, 222, 27, 0xFFFF),
      swordIcon(17, 190, 16, 16, image_sword_02b_pixels),
      swordStat(34, 191, "100", 0xFFFF, 1, TL_DATUM, true, 0x0000),
      armorIcon(100, 190, 16, 16, image_armor_01b_pixels),
      armorStat(117, 191, "100", 0xFFFF, 1, TL_DATUM, true, 0x0000),
      enemyHeartIcon(181, 189, 15, 16, image_cards_hearts_bits, 0xC0E5),
      enemyHp(198, 191, "20", 0xFFFF, 1, TL_DATUM, true, 0x0000),

      // Combat Countdown
      timerBox(100, 219, 51, 17, 0xFFFF),
      timerText(110, 225, "3.00s", 0xFFFF, 1, TL_DATUM, false, 0x0000),

      // Player Section
      levelText(49, 262, "Level XX", 0xFFFF, 1, TL_DATUM, true, 0x0000),
      expBox(37, 273, 76, 13, 0xFFFF),
      expText(49, 291, "23/10000", 0xFFFF, 1, TL_DATUM, true, 0x0000),
      playerHeartIcon(153, 270, 15, 16, image_cards_hearts_bits, 0xC0E5),
      playerHp(171, 271, "20", 0xFFFF, 1, TL_DATUM, true, 0x0000),

      lastTimerUpdate(0),
      timerCountdown(3.00f),
      currentEnemyHp(20),
      currentPlayerHp(20)
{
    addElement(&enemyFrame);
    addElement(&enemyRato);
    addElement(&enemyName);

    addElement(&titleSewers);
    addElement(&navInventory);
    addElement(&navArrow);

    addElement(&statsBar);
    addElement(&swordIcon);
    addElement(&swordStat);
    addElement(&armorIcon);
    addElement(&armorStat);
    addElement(&enemyHeartIcon);
    addElement(&enemyHp);

    addElement(&timerBox);
    addElement(&timerText);

    addElement(&levelText);
    addElement(&expBox);
    addElement(&expText);
    addElement(&playerHeartIcon);
    addElement(&playerHp);
}

void HomeScreen::update(unsigned long now)
{
    if (now - lastTimerUpdate >= 100) // Update timer ~10 times per second
    {
        lastTimerUpdate = now;

        timerCountdown -= 0.10f;
        if (timerCountdown <= 0.00f)
        {
            timerCountdown = 3.00f;

            currentPlayerHp--;
            if (currentPlayerHp <= 0)
            {
                currentPlayerHp = 20;
            }
            playerHp.setText(String(currentPlayerHp));
        }

        char buf[12];
        dtostrf(timerCountdown, 4, 2, buf);
        String s = String(buf) + "s";
        timerText.setText(s);
    }
}
