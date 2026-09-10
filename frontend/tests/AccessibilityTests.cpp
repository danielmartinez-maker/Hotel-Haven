#include "TestFramework.h"
#include "hh/frontend/UiSettings.h"

using namespace hh::frontend;

TEST_CASE("Accessibility settings support required scale matrix") {
    UiSettings settings;
    for (const int scale : {90,100,110,125,150}) {
        EXPECT_TRUE(settings.setScalePercent(scale));
        EXPECT_EQ(settings.scalePercent(), scale);
    }
    EXPECT_FALSE(settings.setScalePercent(137));
}

TEST_CASE("Accessibility never depends on color alone and supports reduced motion") {
    UiSettings settings;
    settings.setReducedMotion(true);
    settings.setInputModality(InputModality::Keyboard);
    EXPECT_TRUE(settings.reducedMotion());
    EXPECT_TRUE(settings.visibleFocusRequired());
    EXPECT_TRUE(settings.requiresTextOrSymbolAlongsideColor());
    EXPECT_TRUE(settings.localizationExpansionFactor() >= 1.35f);
}
