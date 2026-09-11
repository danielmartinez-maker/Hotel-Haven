#include "TestFramework.h"
#include "hh/frontend/GameUiRuntime.h"

using namespace hh::frontend;

TEST_CASE("Game UI runtime caches unchanged snapshots and never advances simulation") {
    int commandCount = 0;
    UiCommand last;
    GameUiRuntime runtime([&](const UiCommand& command) {
        ++commandCount;
        last = command;
        return UiCommandResult{true, "", ""};
    });
    SimulationSnapshot source;
    source.revision = 10;
    source.hud.speed = SimulationSpeed::Paused;
    runtime.update(source);
    const auto firstBuilds = runtime.snapshotBuildCount();
    runtime.update(source);
    EXPECT_EQ(runtime.snapshotBuildCount(), firstBuilds);
    runtime.openManagementPanel(ManagementPanelId::Operations);
    EXPECT_EQ(commandCount, 0);
    const auto result = runtime.dispatchUiCommand({UiCommandType::SaveGame});
    EXPECT_TRUE(result.ok);
    EXPECT_EQ(commandCount, 1);
    EXPECT_EQ(last.type, UiCommandType::SaveGame);
}

TEST_CASE("Cutaway presentation changes are not discarded by the revision cache") {
    GameUiRuntime runtime;
    SimulationSnapshot source;
    source.revision = 22;
    source.hud.cutaway = true;
    runtime.update(source);
    const auto firstBuilds = runtime.snapshotBuildCount();

    source.hud.cutaway = false;
    runtime.update(source);

    EXPECT_EQ(runtime.snapshotBuildCount(), firstBuilds + 1);
    EXPECT_FALSE(runtime.snapshot().hud.cutaway);
}

TEST_CASE("Stable FINAL-07 interfaces maintain UI-only navigation state") {
    GameUiRuntime runtime;
    runtime.setOverlay(OverlayId::Cleanliness);
    runtime.openInspector(88);
    runtime.openManagementPanel(ManagementPanelId::Finance);
    EXPECT_EQ(runtime.activeOverlay(), OverlayId::Cleanliness);
    EXPECT_EQ(runtime.inspectedEntity(), static_cast<EntityId>(88));
    EXPECT_EQ(runtime.activePanel(), ManagementPanelId::Finance);
}
