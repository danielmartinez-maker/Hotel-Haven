#include "hh/frontend/MenuInput.h"

namespace hh::frontend {
namespace {

constexpr std::int16_t kStickThreshold = 16000;

}  // namespace

MenuInputFrame MenuGamepadMapper::update(const MenuGamepadState& state) noexcept {
    if (!state.connected) {
        previousPrevious_ = false;
        previousNext_ = false;
        previousActivate_ = false;
        previousCancel_ = false;
        return {};
    }

    const bool previousDirection =
        state.dpadUp || state.dpadLeft ||
        state.leftStickY > kStickThreshold || state.leftStickX < -kStickThreshold;
    const bool nextDirection =
        state.dpadDown || state.dpadRight ||
        state.leftStickY < -kStickThreshold || state.leftStickX > kStickThreshold;

    MenuInputFrame frame{};
    if (previousDirection && !previousPrevious_) {
        frame.navigationDelta = -1;
    } else if (nextDirection && !previousNext_) {
        frame.navigationDelta = 1;
    }
    frame.activate = state.a && !previousActivate_;
    frame.cancel = state.b && !previousCancel_;

    previousPrevious_ = previousDirection;
    previousNext_ = nextDirection;
    previousActivate_ = state.a;
    previousCancel_ = state.b;
    return frame;
}

}  // namespace hh::frontend
