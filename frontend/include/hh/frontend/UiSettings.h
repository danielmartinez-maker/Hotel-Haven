#pragma once
#include "hh/frontend/GameUiTypes.h"
namespace hh::frontend {
class UiSettings {
public:
    [[nodiscard]] bool setScalePercent(int value) noexcept; [[nodiscard]] int scalePercent() const noexcept { return scalePercent_; }
    void setReducedMotion(bool value) noexcept { reducedMotion_ = value; } [[nodiscard]] bool reducedMotion() const noexcept { return reducedMotion_; }
    void setInputModality(InputModality value) noexcept { inputModality_ = value; } [[nodiscard]] InputModality inputModality() const noexcept { return inputModality_; }
    [[nodiscard]] bool visibleFocusRequired() const noexcept; [[nodiscard]] bool requiresTextOrSymbolAlongsideColor() const noexcept { return true; }
    [[nodiscard]] float localizationExpansionFactor() const noexcept { return 1.40f; }
private: int scalePercent_{100}; bool reducedMotion_{}; InputModality inputModality_{InputModality::Mouse};
};
} // namespace hh::frontend
