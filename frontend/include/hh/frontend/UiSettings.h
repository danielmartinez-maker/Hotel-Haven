#pragma once
#include "hh/frontend/GameUiTypes.h"
#include <array>
#include <cstddef>
#include <optional>
namespace hh::frontend {
class UiSettings {
public:
    [[nodiscard]] bool setScalePercent(int value) noexcept; [[nodiscard]] int scalePercent() const noexcept { return scalePercent_; }
    void setReducedMotion(bool value) noexcept { reducedMotion_ = value; } [[nodiscard]] bool reducedMotion() const noexcept { return reducedMotion_; }
    void setInputModality(InputModality value) noexcept { inputModality_ = value; } [[nodiscard]] InputModality inputModality() const noexcept { return inputModality_; }
    [[nodiscard]] bool visibleFocusRequired() const noexcept; [[nodiscard]] bool requiresTextOrSymbolAlongsideColor() const noexcept { return true; }
    [[nodiscard]] float localizationExpansionFactor() const noexcept { return 1.40f; }
    [[nodiscard]] int keyboardBinding(UiAction action) const noexcept;
    [[nodiscard]] bool setKeyboardBinding(UiAction action, int keyCode) noexcept;
    [[nodiscard]] std::optional<UiAction> actionForKey(int keyCode) const noexcept;
    void resetKeyboardBindings() noexcept;
private:
    static constexpr std::size_t KeyboardActionCount = 7;
    static constexpr std::array<int, KeyboardActionCount> DefaultKeyboardBindings{
        38, 40, 13, 27, 189, 187, 32};
    int scalePercent_{100};
    bool reducedMotion_{};
    InputModality inputModality_{InputModality::Mouse};
    std::array<int, KeyboardActionCount> keyboardBindings_{DefaultKeyboardBindings};
};
} // namespace hh::frontend
