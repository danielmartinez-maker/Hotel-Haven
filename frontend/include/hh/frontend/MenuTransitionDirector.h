#pragma once

#include <cstddef>

#include "hh/frontend/MainMenuCommands.h"

namespace hh::frontend {

struct MenuTransitionState {
    MainMenuItem targetItem{MainMenuItem::Continue};
    float linearProgress{1.0F};
    float easedProgress{1.0F};
    float focusOpacity{1.0F};
    float itemShiftPixels{10.0F};
    bool reducedMotion{};
    bool complete{true};
};

class MenuTransitionDirector {
public:
    void retarget(MainMenuItem item, bool reducedMotion) noexcept;
    [[nodiscard]] MenuTransitionState update(float deltaSeconds) noexcept;

    [[nodiscard]] MainMenuItem targetItem() const noexcept { return targetItem_; }
    [[nodiscard]] std::size_t queuedTransitionCount() const noexcept { return 0U; }

private:
    MainMenuItem targetItem_{MainMenuItem::Continue};
    float elapsedSeconds_{1.0F};
    float durationSeconds_{0.50F};
    bool reducedMotion_{};
};

}  // namespace hh::frontend
