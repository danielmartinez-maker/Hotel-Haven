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

TEST_CASE("Keyboard controls can be remapped without ambiguous duplicate bindings") {
    UiSettings settings;
    const int originalPause = settings.keyboardBinding(UiAction::PauseToggle);
    const int originalCancel = settings.keyboardBinding(UiAction::Cancel);
    EXPECT_TRUE(originalPause > 0);
    EXPECT_TRUE(originalCancel > 0);
    EXPECT_TRUE(originalPause != originalCancel);

    EXPECT_TRUE(settings.setKeyboardBinding(UiAction::PauseToggle, 'P'));
    EXPECT_EQ(settings.keyboardBinding(UiAction::PauseToggle), static_cast<int>('P'));
    EXPECT_FALSE(settings.setKeyboardBinding(UiAction::Cancel, 'P'));
    EXPECT_EQ(settings.keyboardBinding(UiAction::Cancel), originalCancel);
    EXPECT_FALSE(settings.setKeyboardBinding(UiAction::Cancel, 0));
    EXPECT_FALSE(settings.setKeyboardBinding(UiAction::Cancel, 256));

    const auto remapped = settings.actionForKey('P');
    EXPECT_TRUE(remapped.has_value());
    EXPECT_EQ(*remapped, UiAction::PauseToggle);
    EXPECT_FALSE(settings.actionForKey(originalPause).has_value());

    settings.resetKeyboardBindings();
    EXPECT_EQ(settings.keyboardBinding(UiAction::PauseToggle), originalPause);
    EXPECT_EQ(settings.keyboardBinding(UiAction::Cancel), originalCancel);
    const auto restored = settings.actionForKey(originalPause);
    EXPECT_TRUE(restored.has_value());
    EXPECT_EQ(*restored, UiAction::PauseToggle);
}
