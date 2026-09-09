#include "hh/frontend/MenuSceneController.h"

#include <array>
#include <cmath>

namespace hh::frontend {

AmbientPresentationScheduler::AmbientPresentationScheduler(std::uint32_t seed)
    : random_(seed),
      eventDistribution_({18, 16, 13, 14, 12, 19, 8}) {}

AmbientPresentationEvent AmbientPresentationScheduler::nextEvent() {
    const int index = eventDistribution_(random_);
    const auto type = static_cast<AmbientEventType>(index);
    return AmbientPresentationEvent{type, intervalDistribution_(random_)};
}

CameraPresentationPose MenuSceneController::cameraPose(
    float elapsedSeconds,
    const MenuTransitionState& transition) const noexcept {
    if (reducedMotion_) {
        return {};
    }

    CameraPresentationPose result;
    const bool suppressIdle = transition.targetItem == MainMenuItem::Settings;
    result.idleYawDegrees = suppressIdle ? 0.0F : std::sin(elapsedSeconds * 0.07F) * 2.0F;
    result.idleZoomScale = suppressIdle ? 1.0F : 1.0F + std::sin(elapsedSeconds * 0.11F) * 0.02F;

    const float amount = transition.easedProgress;
    float hoverYaw = 0.0F;
    float hoverZoom = 1.0F;
    float targetX = 0.0F;
    float targetZ = 0.0F;

    switch (transition.targetItem) {
    case MainMenuItem::Continue:
        hoverZoom = 1.0F - 0.035F * amount;
        break;
    case MainMenuItem::NewHotel:
        hoverZoom = 1.0F + 0.04F * amount;
        break;
    case MainMenuItem::LoadHotel:
        hoverYaw = 1.6F * amount;
        targetX = 0.35F * amount;
        break;
    case MainMenuItem::Scenarios:
        hoverYaw = -1.6F * amount;
        targetX = -0.35F * amount;
        break;
    case MainMenuItem::Sandbox:
        hoverZoom = 1.0F + 0.05F * amount;
        targetZ = 0.55F * amount;
        break;
    case MainMenuItem::Settings:
        break;
    case MainMenuItem::Credits:
        hoverZoom = 1.0F + 0.06F * amount;
        break;
    case MainMenuItem::Quit:
        break;
    }

    result.yawOffsetDegrees = result.idleYawDegrees + hoverYaw;
    result.targetXOffset = targetX;
    result.targetZOffset = targetZ;
    result.zoomScale = result.idleZoomScale * hoverZoom;
    return result;
}

}  // namespace hh::frontend
