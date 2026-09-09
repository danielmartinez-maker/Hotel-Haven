#pragma once

#include <cstdint>
#include <random>

#include "hh/frontend/MenuTransitionDirector.h"

namespace hh::frontend {

struct CameraPresentationPose {
    float idleYawDegrees{};
    float idleZoomScale{1.0F};
    float yawOffsetDegrees{};
    float targetXOffset{};
    float targetZOffset{};
    float zoomScale{1.0F};
};

enum class AmbientEventType : std::uint8_t {
    GuestArrival,
    CheckInInteraction,
    BellhopLuggageRun,
    ElevatorArrival,
    CleaningPass,
    LobbyConversation,
    VehicleDropoff,
};

struct AmbientPresentationEvent {
    AmbientEventType type{AmbientEventType::GuestArrival};
    float secondsUntilNext{5.0F};
    friend bool operator==(const AmbientPresentationEvent&, const AmbientPresentationEvent&) = default;
};

class AmbientPresentationScheduler {
public:
    explicit AmbientPresentationScheduler(std::uint32_t seed);
    [[nodiscard]] AmbientPresentationEvent nextEvent();

private:
    std::mt19937 random_;
    std::discrete_distribution<int> eventDistribution_;
    std::uniform_real_distribution<float> intervalDistribution_{5.0F, 15.0F};
};

class MenuSceneController {
public:
    void setReducedMotion(bool enabled) noexcept { reducedMotion_ = enabled; }
    [[nodiscard]] bool reducedMotion() const noexcept { return reducedMotion_; }
    [[nodiscard]] CameraPresentationPose cameraPose(
        float elapsedSeconds,
        const MenuTransitionState& transition) const noexcept;

private:
    bool reducedMotion_{};
};

}  // namespace hh::frontend
