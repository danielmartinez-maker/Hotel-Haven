#pragma once
#include "hh/frontend/GameUiTypes.h"
#include <array>
#include <cstddef>
#include <optional>

namespace hh::frontend {

class UiSettings {
public:
    static constexpr std::size_t KeyboardActionCount = 7;
    using KeyboardBindings = std::array<int, KeyboardActionCount>;

    [[nodiscard]] bool setScalePercent(int value) noexcept;
    [[nodiscard]] int scalePercent() const noexcept { return scalePercent_; }

    void setReducedMotion(bool value) noexcept { reducedMotion_ = value; }
    [[nodiscard]] bool reducedMotion() const noexcept { return reducedMotion_; }

    void setInputModality(InputModality value) noexcept { inputModality_ = value; }
    [[nodiscard]] InputModality inputModality() const noexcept { return inputModality_; }

    [[nodiscard]] bool visibleFocusRequired() const noexcept;
    [[nodiscard]] bool requiresTextOrSymbolAlongsideColor() const noexcept { return true; }
    [[nodiscard]] float localizationExpansionFactor() const noexcept { return 1.40f; }

    [[nodiscard]] int keyboardBinding(UiAction action) const noexcept;
    [[nodiscard]] const KeyboardBindings& keyboardBindings() const noexcept {
        return keyboardBindings_;
    }
    [[nodiscard]] bool setKeyboardBinding(UiAction action, int keyCode) noexcept;
    [[nodiscard]] bool setKeyboardBindings(const KeyboardBindings& bindings) noexcept;
    [[nodiscard]] std::optional<UiAction> actionForKey(int keyCode) const noexcept;
    void resetKeyboardBindings() noexcept;

private:
    static constexpr KeyboardBindings DefaultKeyboardBindings{
        38, 40, 13, 27, 189, 187, 32};

    int scalePercent_{100};
    bool reducedMotion_{};
    InputModality inputModality_{InputModality::Mouse};
    KeyboardBindings keyboardBindings_{DefaultKeyboardBindings};
};

} // namespace hh::frontend
