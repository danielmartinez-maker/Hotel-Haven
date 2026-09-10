#include "TestFramework.h"
#include "hh/frontend/AlertCenter.h"
#include "hh/frontend/BuildCatalog.h"
#include "hh/frontend/GameUiRuntime.h"
#include "hh/frontend/InspectorModel.h"

#include <cstdint>
#include <string>

using namespace hh::frontend;

namespace {
std::uint64_t authorityFingerprint(const SimulationSnapshot &snapshot) {
    std::uint64_t hash = 1469598103934665603ULL;
    auto mix = [&](std::uint64_t value) {
        hash ^= value;
        hash *= 1099511628211ULL;
    };
    mix(snapshot.revision);
    mix(static_cast<std::uint64_t>(snapshot.hud.cashCents));
    mix(static_cast<std::uint64_t>(snapshot.hud.day));
    mix(static_cast<std::uint64_t>(snapshot.hud.hour));
    mix(static_cast<std::uint64_t>(snapshot.hud.alertCount));
    mix(snapshot.entities.size());
    for (const auto &entity : snapshot.entities)
        mix(entity.id);
    mix(snapshot.buildCatalog.size());
    mix(snapshot.overlays.size());
    mix(snapshot.alerts.size());
    mix(snapshot.objectives.items.size());
    mix(static_cast<std::uint64_t>(snapshot.operations.housekeepingBacklog));
    mix(static_cast<std::uint64_t>(snapshot.operations.roomServiceQueue));
    mix(static_cast<std::uint64_t>(snapshot.economy.kpis.cashCents));
    return hash;
}

SimulationSnapshot richSnapshot() {
    SimulationSnapshot snapshot;
    snapshot.revision = 1;
    snapshot.hud.cashCents = 5'000'000;
    snapshot.hud.day = 10;
    snapshot.hud.hour = 14;
    snapshot.hud.minute = 30;
    snapshot.hud.speed = SimulationSpeed::OneX;
    snapshot.hud.occupancyPermille = 820;
    snapshot.hud.satisfactionPermille = 790;
    snapshot.hud.reputationPermille = 810;
    snapshot.hud.alertCount = 12;
    snapshot.hud.activeFloor = 2;

    for (EntityId id = 1; id <= 256; ++id) {
        UiEntitySnapshot entity;
        entity.id = id;
        entity.kind = static_cast<InspectorKind>(id % 10);
        entity.title = "Entity " + std::to_string(id);
        entity.fields.push_back({"State", id % 2 == 0 ? "Ready" : "Working"});
        entity.fields.push_back({"Value", std::to_string(id * 10)});
        if (id % 17 == 0)
            entity.diagnostics.push_back({"PRESSURE", "Stress diagnostic", id, 0});
        snapshot.entities.push_back(std::move(entity));
    }

    for (int index = 0; index < 64; ++index)
        snapshot.buildCatalog.push_back({"item_" + std::to_string(index),
                                         "Item " + std::to_string(index),
                                         index % 2 == 0 ? "Rooms" : "Service",
                                         1000 + index * 100,
                                         {"material"}, 5 + index});

    for (int overlay = static_cast<int>(OverlayId::Cleanliness);
         overlay <= static_cast<int>(OverlayId::Revenue); ++overlay) {
        OverlaySnapshot value;
        value.id = static_cast<OverlayId>(overlay);
        value.unit = "scaled";
        value.minValue = 0;
        value.maxValue = 1000;
        for (EntityId id = 1; id <= 64; ++id)
            value.samples.push_back({id, static_cast<std::int64_t>((id * overlay) % 1001),
                                     std::to_string((id * overlay) % 1001), "normal"});
        snapshot.overlays.push_back(std::move(value));
    }

    snapshot.operations.scheduledStaff = 120;
    snapshot.operations.activeStaff = 80;
    snapshot.operations.housekeepingBacklog = 30;
    snapshot.operations.cleanLinenUnits = 140;
    snapshot.operations.roomServiceQueue = 15;
    snapshot.operations.foodTickets = 22;
    snapshot.operations.eventCount = 3;
    snapshot.operations.amenityCapacityUsed = 20;
    snapshot.operations.amenityCapacityTotal = 40;
    snapshot.operations.serviceLevelPermille = 880;
    snapshot.economy.kpis.cashCents = 5'000'000;
    snapshot.economy.kpis.todayOccupancyPermille = 820;
    snapshot.economy.kpis.adrCents = 18'500;
    snapshot.economy.kpis.revParCents = 15'170;
    snapshot.economy.kpis.gopCents = 900'000;

    for (std::uint64_t id = 1; id <= 32; ++id)
        snapshot.alerts.push_back({id, id % 7 == 0 ? AlertSeverity::Critical : AlertSeverity::Warning,
                                   static_cast<EntityId>((id % 256) + 1), "STRESS_ALERT",
                                   "Operational stress alert", id > 1 ? id - 1 : 0, false});
    for (std::uint64_t id = 1; id <= 16; ++id)
        snapshot.objectives.items.push_back({id, "Objective " + std::to_string(id),
                                             static_cast<std::int64_t>(id), 20,
                                             false, "", "", true});
    return snapshot;
}
} // namespace

TEST_CASE("FINAL-07 UI survives 100k model controller and snapshot actions") {
    SimulationSnapshot source = richSnapshot();
    std::size_t dispatched = 0;
    GameUiRuntime runtime([&](const UiCommand &command) {
        ++dispatched;
        if (command.type == UiCommandType::None)
            return UiCommandResult{false, "INVALID", "No-op command"};
        return UiCommandResult{true, "", ""};
    });
    runtime.update(source);
    BuildToolController buildTool;

    std::size_t expectedBuilds = runtime.snapshotBuildCount();
    for (std::uint64_t iteration = 0; iteration < 100'000; ++iteration) {
        switch (iteration % 8) {
        case 0: {
            const auto before = authorityFingerprint(source);
            const auto overlayCount = static_cast<std::uint64_t>(OverlayId::Revenue) + 1;
            runtime.setOverlay(static_cast<OverlayId>(iteration % overlayCount));
            EXPECT_EQ(authorityFingerprint(source), before);
            break;
        }
        case 1: {
            const auto before = authorityFingerprint(source);
            const auto panelCount = static_cast<std::uint64_t>(ManagementPanelId::Supplies) + 1;
            runtime.openManagementPanel(static_cast<ManagementPanelId>(iteration % panelCount));
            EXPECT_EQ(authorityFingerprint(source), before);
            break;
        }
        case 2: {
            const auto before = authorityFingerprint(source);
            const EntityId id = static_cast<EntityId>((iteration % source.entities.size()) + 1);
            runtime.openInspector(id);
            const auto presentation = InspectorModel::compose(id, runtime.snapshot().entities);
            EXPECT_TRUE(presentation.has_value());
            EXPECT_EQ(presentation->entityId, id);
            EXPECT_EQ(authorityFingerprint(source), before);
            break;
        }
        case 3: {
            const auto before = authorityFingerprint(source);
            BuildPlacementPreview preview;
            preview.requestId = iteration + 1;
            preview.itemId = "item_" + std::to_string(iteration % 64);
            preview.valid = (iteration & 16ULL) == 0;
            preview.reasonCode = preview.valid ? "" : "BLOCKED";
            preview.reasonText = preview.valid ? "" : "Placement blocked";
            preview.costCents = 10'000;
            preview.laborMinutes = 15;
            preview.floor = static_cast<int>(iteration % 8);
            preview.x = static_cast<int>(iteration % 128);
            preview.y = static_cast<int>((iteration / 2) % 128);
            preview.rotationQuarterTurns = static_cast<int>(iteration % 4);
            buildTool.selectItem(preview.itemId);
            buildTool.applyAuthoritativePreview(preview);
            EXPECT_EQ(buildTool.canConfirm(), preview.valid);
            const auto confirm = buildTool.confirmCommand();
            EXPECT_EQ(confirm.has_value(), preview.valid);
            EXPECT_EQ(buildTool.rotateCommand().type, UiCommandType::BuildRotate);
            EXPECT_EQ(buildTool.cancelCommand().type, UiCommandType::BuildCancel);
            EXPECT_EQ(authorityFingerprint(source), before);
            break;
        }
        case 4: {
            const auto before = authorityFingerprint(source);
            const auto result = runtime.dispatchUiCommand(
                {iteration % 2 == 0 ? UiCommandType::SaveGame : UiCommandType::OpenSettings});
            EXPECT_TRUE(result.ok);
            EXPECT_EQ(authorityFingerprint(source), before);
            break;
        }
        case 5: {
            ++source.revision;
            source.hud.minute = static_cast<int>((source.hud.minute + 1) % 60);
            source.operations.housekeepingBacklog = static_cast<int>(iteration % 500);
            source.operations.roomServiceQueue = static_cast<int>(iteration % 100);
            source.economy.kpis.cashCents += static_cast<std::int64_t>(iteration % 17) - 8;
            const auto before = authorityFingerprint(source);
            runtime.update(source);
            ++expectedBuilds;
            EXPECT_EQ(runtime.snapshotBuildCount(), expectedBuilds);
            EXPECT_EQ(runtime.snapshot().revision, source.revision);
            EXPECT_EQ(authorityFingerprint(source), before);
            break;
        }
        case 6: {
            const auto before = authorityFingerprint(source);
            const auto buildsBefore = runtime.snapshotBuildCount();
            runtime.update(source);
            EXPECT_EQ(runtime.snapshotBuildCount(), buildsBefore);
            EXPECT_EQ(authorityFingerprint(source), before);
            break;
        }
        case 7: {
            const auto before = authorityFingerprint(source);
            const auto stale = InspectorModel::compose(999'999'999ULL, runtime.snapshot().entities);
            EXPECT_FALSE(stale.has_value());
            EXPECT_EQ(authorityFingerprint(source), before);
            break;
        }
        }
    }

    EXPECT_TRUE(dispatched >= 12'000);
    EXPECT_EQ(runtime.snapshot().revision, source.revision);
    EXPECT_TRUE(runtime.snapshot().operations.housekeepingBacklog >= 0);
    EXPECT_TRUE(runtime.snapshot().economy.kpis.adrCents >= 0);
}

TEST_CASE("Alert center dedupe and resolved history stay bounded under 20k transitions") {
    constexpr std::size_t kHistoryLimit = 200;
    AlertCenter center(kHistoryLimit);
    for (std::uint64_t id = 1; id <= 10'000; ++id) {
        const AlertSnapshot active{id, id % 11 == 0 ? AlertSeverity::Critical : AlertSeverity::Warning,
                                   static_cast<EntityId>((id % 256) + 1), "PRESSURE",
                                   "Repeated causal alert", id > 1 ? id - 1 : 0, false};
        center.ingest({active});
        center.ingest({active});
        EXPECT_TRUE(center.active().size() <= 1);
        auto resolved = active;
        resolved.resolved = true;
        center.ingest({resolved});
        EXPECT_TRUE(center.active().empty());
        EXPECT_TRUE(center.resolvedHistory().size() <= kHistoryLimit);
    }
    EXPECT_EQ(center.resolvedHistory().size(), kHistoryLimit);
    const auto navigation = center.navigationFor(10'000);
    EXPECT_TRUE(navigation.has_value());
    EXPECT_EQ(navigation->type, UiCommandType::OpenInspector);
}
