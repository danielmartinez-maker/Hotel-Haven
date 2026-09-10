#pragma once
#include "hh/frontend/GameHudModel.h"
#include <optional>
namespace hh::frontend {
class GameHudController {
public:
    explicit GameHudController(GameHudModel& model) noexcept : model_(model) {}
    void setFocus(HudFocus focus) noexcept { focus_ = focus; }
    [[nodiscard]] HudFocus focus() const noexcept { return focus_; }
    [[nodiscard]] std::optional<UiCommand> handleAction(UiAction action) const;
private:
    [[nodiscard]] SimulationSpeed nextSpeed(bool increase) const noexcept;
    GameHudModel& model_; HudFocus focus_{HudFocus::Time};
};
} // namespace hh::frontend
