#pragma once
#include "hh/frontend/GameUiTypes.h"
#include <string>
namespace hh::frontend {
class GameHudModel {
public:
    void update(const HudSnapshot& snapshot);
    [[nodiscard]] const HudSnapshot& snapshot() const noexcept { return snapshot_; }
    [[nodiscard]] std::string cashText() const; [[nodiscard]] std::string timeText() const;
    [[nodiscard]] std::string occupancyText() const; [[nodiscard]] std::string satisfactionText() const;
    [[nodiscard]] std::string reputationText() const;
    [[nodiscard]] SimulationSpeed speed() const noexcept { return snapshot_.speed; }
    [[nodiscard]] int alertCount() const noexcept { return snapshot_.alertCount; }
    [[nodiscard]] int activeFloor() const noexcept { return snapshot_.activeFloor; }
    [[nodiscard]] bool cutaway() const noexcept { return snapshot_.cutaway; }
private: HudSnapshot snapshot_{};
};
} // namespace hh::frontend
