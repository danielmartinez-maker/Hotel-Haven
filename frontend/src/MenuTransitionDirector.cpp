#include "hh/frontend/MenuTransitionDirector.h"

#include <algorithm>

namespace hh::frontend {
namespace {

float smoothStep(float value) noexcept {
    const float t = std::clamp(value, 0.0F, 1.0F);
    return t * t * (3.0F - 2.0F * t);
}

}  // namespace

void MenuTransitionDirector::retarget(MainMenuItem item, bool reducedMotion) noexcept {
    targetItem_ = item;
    reducedMotion_ = reducedMotion;
    elapsedSeconds_ = 0.0F;
    durationSeconds_ = reducedMotion ? 0.16F : 0.50F;
}

MenuTransitionState MenuTransitionDirector::update(float deltaSeconds) noexcept {
    elapsedSeconds_ += std::max(deltaSeconds, 0.0F);
    const float progress = durationSeconds_ > 0.0F
                               ? std::clamp(elapsedSeconds_ / durationSeconds_, 0.0F, 1.0F)
                               : 1.0F;
    const float eased = smoothStep(progress);

    MenuTransitionState result;
    result.targetItem = targetItem_;
    result.linearProgress = progress;
    result.easedProgress = eased;
    result.focusOpacity = eased;
    result.itemShiftPixels = reducedMotion_ ? 0.0F : 10.0F * eased;
    result.reducedMotion = reducedMotion_;
    result.complete = progress >= 1.0F;
    return result;
}

}  // namespace hh::frontend
