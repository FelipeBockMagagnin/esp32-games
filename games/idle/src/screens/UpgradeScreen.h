#include "Screen.h"
#include "../ui/Text.h"
#include "../ui/Box.h"
#include "../ui/Image.h"
#include "../assets/Assets.h"
#include <ui/CoinDisplay.h>
#include <ui/UpgradeRow.h>

class UpgradeScreen : public Screen
{
private:
    Text titleText;

    Image headerLeftIcon;
    Text backText;

    Image headerRightIcon;
    Text nextText;

    CoinDisplay cooperDisplay;
    CoinDisplay silverDisplay;
    CoinDisplay goldDisplay;

    UpgradeRow upgradeRow;

public:
    UpgradeScreen();

    void update(unsigned long now) override;
};
