#pragma once
#include "hh/frontend/GameUiTypes.h"
#include <cstddef>
#include <functional>
namespace hh::frontend {
class GameUiRuntime {
public:
    using CommandSink = std::function<UiCommandResult(const UiCommand&)>;
    explicit GameUiRuntime(CommandSink commandSink = {}); void update(const SimulationSnapshot& snapshot);
    [[nodiscard]] const GameUiSnapshot& snapshot() const noexcept { return snapshot_; }
    [[nodiscard]] UiCommandResult dispatchUiCommand(const UiCommand& command);
    void setOverlay(OverlayId overlay) noexcept { activeOverlay_ = overlay; }
    void openInspector(EntityId entityId) noexcept { inspectedEntity_ = entityId; }
    void openManagementPanel(ManagementPanelId panel) noexcept { activePanel_ = panel; }
    [[nodiscard]] OverlayId activeOverlay() const noexcept { return activeOverlay_; }
    [[nodiscard]] EntityId inspectedEntity() const noexcept { return inspectedEntity_; }
    [[nodiscard]] ManagementPanelId activePanel() const noexcept { return activePanel_; }
    [[nodiscard]] std::size_t snapshotBuildCount() const noexcept { return snapshotBuildCount_; }
private:
    CommandSink commandSink_; GameUiSnapshot snapshot_{}; OverlayId activeOverlay_{OverlayId::None}; EntityId inspectedEntity_{}; ManagementPanelId activePanel_{ManagementPanelId::None}; std::size_t snapshotBuildCount_{};
};
} // namespace hh::frontend
