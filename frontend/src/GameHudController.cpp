#include "hh/frontend/GameHudController.h"
#include <array>
namespace hh::frontend {
SimulationSpeed GameHudController::nextSpeed(bool increase) const noexcept {
    constexpr std::array<SimulationSpeed, 5> speeds{SimulationSpeed::Paused, SimulationSpeed::OneX, SimulationSpeed::TwoX, SimulationSpeed::FourX, SimulationSpeed::EightX};
    std::size_t index = 0; for (std::size_t i = 0; i < speeds.size(); ++i) if (speeds[i] == model_.speed()) { index = i; break; }
    if (increase && index + 1 < speeds.size()) ++index; if (!increase && index > 0) --index; return speeds[index];
}
std::optional<UiCommand> GameHudController::handleAction(UiAction action) const {
    if (action == UiAction::SpeedUp) return UiCommand{UiCommandType::SetSimulationSpeed, 0, static_cast<std::int64_t>(nextSpeed(true))};
    if (action == UiAction::SpeedDown) return UiCommand{UiCommandType::SetSimulationSpeed, 0, static_cast<std::int64_t>(nextSpeed(false))};
    if (action == UiAction::PauseToggle) { const auto speed = model_.speed() == SimulationSpeed::Paused ? SimulationSpeed::OneX : SimulationSpeed::Paused; return UiCommand{UiCommandType::SetSimulationSpeed, 0, static_cast<std::int64_t>(speed)}; }
    if (action != UiAction::Activate) return std::nullopt;
    if (focus_ == HudFocus::Speed) return UiCommand{UiCommandType::SetSimulationSpeed, 0, static_cast<std::int64_t>(SimulationSpeed::OneX)};
    if (focus_ == HudFocus::Alerts) return UiCommand{UiCommandType::OpenManagementPanel, 0, static_cast<std::int64_t>(ManagementPanelId::Alerts)};
    return std::nullopt;
}
} // namespace hh::frontend
