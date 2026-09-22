#include "TestFramework.h"
#include "hh/frontend/KeyBindingEditor.h"
#include "hh/frontend/UiSettings.h"
#include "hh/frontend/UiSettingsCodec.h"

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

TEST_CASE("Key binding editor captures, validates, and cancels player rebinding") {
    UiSettings settings;
    KeyBindingEditor editor;
    EXPECT_FALSE(editor.capturing());
    EXPECT_EQ(editor.capture(settings, 'P'), KeyBindingCaptureResult::Idle);

    editor.begin(UiAction::PauseToggle);
    EXPECT_TRUE(editor.capturing());
    EXPECT_TRUE(editor.pendingAction().has_value());
    EXPECT_EQ(*editor.pendingAction(), UiAction::PauseToggle);

    const int cancelKey = settings.keyboardBinding(UiAction::Cancel);
    EXPECT_EQ(editor.capture(settings, cancelKey), KeyBindingCaptureResult::Duplicate);
    EXPECT_TRUE(editor.capturing());
    EXPECT_EQ(editor.capture(settings, 0), KeyBindingCaptureResult::Invalid);
    EXPECT_TRUE(editor.capturing());

    EXPECT_EQ(editor.capture(settings, 'P'), KeyBindingCaptureResult::Applied);
    EXPECT_FALSE(editor.capturing());
    EXPECT_EQ(settings.keyboardBinding(UiAction::PauseToggle), static_cast<int>('P'));

    editor.begin(UiAction::Activate);
    editor.cancel();
    EXPECT_FALSE(editor.capturing());
}

TEST_CASE("Key binding editor exposes every remappable action exactly once") {
    EXPECT_EQ(EditableKeyBindings.size(), std::size_t{7});
    std::array<bool, 7> seen{};
    for (const auto& descriptor : EditableKeyBindings) {
        const auto index = static_cast<std::size_t>(descriptor.action);
        EXPECT_TRUE(index < seen.size());
        EXPECT_FALSE(seen[index]);
        EXPECT_FALSE(descriptor.label.empty());
        seen[index] = true;
    }
    for (const bool present : seen)
        EXPECT_TRUE(present);
}

TEST_CASE("UI preferences round-trip scale motion and swapped key bindings") {
    UiSettings source;
    EXPECT_TRUE(source.setScalePercent(150));
    source.setReducedMotion(true);

    auto bindings = source.keyboardBindings();
    const int previous = bindings[0];
    bindings[0] = bindings[1];
    bindings[1] = previous;
    EXPECT_TRUE(source.setKeyboardBindings(bindings));

    const std::string encoded = serializeUiSettings(source);
    UiSettings restored;
    restored.setInputModality(InputModality::Controller);
    EXPECT_TRUE(deserializeUiSettings(encoded, restored));

    EXPECT_EQ(restored.scalePercent(), 150);
    EXPECT_TRUE(restored.reducedMotion());
    EXPECT_EQ(restored.inputModality(), InputModality::Controller);
    for (std::size_t index = 0; index < bindings.size(); ++index)
        EXPECT_EQ(restored.keyboardBindings()[index], bindings[index]);
}

TEST_CASE("UI preference decoding rejects corrupt state transactionally") {
    UiSettings settings;
    EXPECT_TRUE(settings.setScalePercent(125));
    settings.setReducedMotion(true);
    EXPECT_TRUE(settings.setKeyboardBinding(UiAction::PauseToggle, 'P'));
    const std::string before = serializeUiSettings(settings);

    EXPECT_FALSE(deserializeUiSettings(
        "HHUI2 100 0 38 40 13 27 189 187 32\n", settings));
    EXPECT_EQ(serializeUiSettings(settings), before);

    EXPECT_FALSE(deserializeUiSettings(
        "HHUI1 100 0 38 38 13 27 189 187 32\n", settings));
    EXPECT_EQ(serializeUiSettings(settings), before);

    EXPECT_FALSE(deserializeUiSettings(
        "HHUI1 100 0 38 40 13 27 189 187 256\n", settings));
    EXPECT_EQ(serializeUiSettings(settings), before);

    EXPECT_FALSE(deserializeUiSettings(
        "HHUI1 100 0 38 40 13 27 189 187 32 trailing\n", settings));
    EXPECT_EQ(serializeUiSettings(settings), before);
}
