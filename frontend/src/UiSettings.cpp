#include "hh/frontend/UiSettings.h"
#include <algorithm>
#include <array>
namespace hh::frontend {
bool UiSettings::setScalePercent(int value) noexcept { constexpr std::array<int, 5> allowed{90,100,110,125,150}; if (std::find(allowed.begin(), allowed.end(), value) == allowed.end()) return false; scalePercent_ = value; return true; }
bool UiSettings::visibleFocusRequired() const noexcept { return inputModality_ == InputModality::Keyboard || inputModality_ == InputModality::Controller; }
} // namespace hh::frontend
