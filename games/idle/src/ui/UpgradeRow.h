#ifndef UI_UPGRADE_ROW_H
#define UI_UPGRADE_ROW_H

#include "UI.h"

class UpgradeRow : public UI
{
public:
    String upgradeName;
    String upgradeDescription;

    uint64_t copperPrice;
    uint64_t silverPrice;
    uint64_t goldPrice;

    String buyCount;

    bool selected;
    bool affordable;

    UpgradeRow(int16_t x = 0, int16_t y = 0, const String &upgradeName = "", const String &upgradeDescription = "")
        : UI(x, y, 230, 34),
          upgradeName(upgradeName),
          upgradeDescription(upgradeDescription),
          copperPrice(0),
          silverPrice(0),
          goldPrice(0),
          buyCount("0"),
          selected(false),
          affordable(false)
    {
    }

    void setDescription(const String &description)
    {
        if (upgradeDescription != description)
        {
            upgradeDescription = description;
            markDirty();
        }
    }

    void setSelected(bool isSelected)
    {
        if (selected != isSelected)
        {
            selected = isSelected;
            markDirty();
        }
    }

    void setAffordable(bool canBuy)
    {
        if (affordable != canBuy)
        {
            affordable = canBuy;
            markDirty();
        }
    }

    void setCopperPrice(uint64_t newPrice)
    {
        if (copperPrice != newPrice)
        {
            copperPrice = newPrice;
            markDirty();
        }
    }

    void setSilverPrice(uint64_t newPrice)
    {
        if (silverPrice != newPrice)
        {
            silverPrice = newPrice;
            markDirty();
        }
    }

    void setGoldPrice(uint64_t newPrice)
    {
        if (goldPrice != newPrice)
        {
            goldPrice = newPrice;
            markDirty();
        }
    }

    void setBuyCount(const String &count)
    {
        if (buyCount != count)
        {
            buyCount = count;
            markDirty();
        }
    }

    void setBuyCount(uint32_t count)
    {
        String s = String(count);
        if (buyCount != s)
        {
            buyCount = s;
            markDirty();
        }
    }

    void draw(TFT_eSPI &tft) override;
};

#endif // UI_UPGRADE_ROW_H

