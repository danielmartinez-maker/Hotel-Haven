#include "hh/frontend/UiSettings.h"
#include <algorithm>
#include <array>
namespace hh::frontend {
bool UiSettings::setScalePercent(int value) noexcept { constexpr std::array<int, 5> allowed{90,100,110,125,150}; if (std::find(allowed.begin(), allowed.end(), value) == allowed.end()) return false; scalePercent_ = value; return true; }
bool UiSettings::visibleFocusRequired() const noexcept { return inputModality_ == InputModality::Keyboard || inputModality_ == InputModality::Controller; }
int UiSettings::keyboardBinding(UiAction action) const noexcept {
    const auto index = static_cast<std::size_t>(action);
    return index < keyboardBindings_.size() ? keyboardBindings_[index] : 0;
}
bool UiSettings::setKeyboardBinding(UiAction action, int keyCode) noexcept {
    const auto index = static_cast<std::size_t>(action);
    if (index >= keyboardBindings_.size() || keyCode <= 0 || keyCode > 255)
        return false;
    for (std::size_t other = 0; other < keyboardBindings_.size(); ++other) {
        if (other != index && keyboardBindings_[other] == keyCode)
            return false;
    }
    keyboardBindings_[index] = keyCode;
    return true;
}
void UiSettings::resetKeyboardBindings() noexcept {
    keyboardBindings_ = DefaultKeyboardBindings;
}
} // namespace hh::frontend
