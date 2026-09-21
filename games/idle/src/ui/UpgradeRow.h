#include "UI.h"

class UpgradeRow : public UI
{
public:
    int16_t x;
    int16_t y;
    String upgradeName;
    String upgradeDescription;

    String cooperPrice;
    String silverPrice;
    String goldPrice;

    String buyCount;

    UpgradeRow(int16_t x, int16_t y, String upgradeName, String upgradeDescription)
        : UI(x, y),
          x(x),
          y(y),
          upgradeName(upgradeName),
          upgradeDescription(upgradeDescription)
    {
    }

    void setCooperPrice(const long newText)
    {
        if (cooperPrice != String(newText))
        {
            cooperPrice = String(newText);
            markDirty();
        }
    }

    void setSilverPrice(const long newText)
    {
        if (silverPrice != String(newText))
        {
            silverPrice = String(newText);
            markDirty();
        }
    }

    void setGoldPrice(const long newText)
    {
        if (goldPrice != String(newText))
        {
            goldPrice = String(newText);
            markDirty();
        }
    }

    void draw(TFT_eSPI &tft) override;
};
