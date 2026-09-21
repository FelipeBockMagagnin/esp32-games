#include "UI.h"

#include "../enum/OreEnum.h"

class CoinDisplay : public UI
{
public:
    int16_t x;
    int16_t y;
    String text;
    int16_t oreType;

    CoinDisplay(int16_t x, int16_t y, String text, int16_t oreType)
        : UI(x, y),
          x(x),
          y(y),
          text(text),
          oreType(oreType)
    {
    }

    void draw(TFT_eSPI &tft) override;
};
