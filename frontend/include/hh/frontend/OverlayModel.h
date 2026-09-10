#pragma once
#include "hh/frontend/GameUiTypes.h"
#include <optional>
#include <string>
#include <vector>
namespace hh::frontend {
struct OverlayDescriptor { OverlayId id{OverlayId::None}; std::string label; std::string unit; std::int64_t minValue{}; std::int64_t maxValue{}; std::string lowSemantic; std::string highSemantic; };
class OverlayModel {
public:
    [[nodiscard]] static std::vector<OverlayDescriptor> requiredDescriptors();
    void bind(const OverlaySnapshot& snapshot) { snapshot_ = snapshot; }
    [[nodiscard]] const OverlaySnapshot& snapshot() const noexcept { return snapshot_; }
    [[nodiscard]] std::optional<OverlaySample> sampleFor(EntityId entityId) const;
    [[nodiscard]] bool hasNonColorSemantics() const noexcept;
private: OverlaySnapshot snapshot_{};
};
} // namespace hh::frontend
