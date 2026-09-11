#include "TestFramework.h"
#include "hh/frontend/AlertCenter.h"

using namespace hh::frontend;

TEST_CASE("Alert center deduplicates stable alerts and preserves causal navigation") {
    AlertCenter center(4);
    const std::vector<AlertSnapshot> snapshot{
        {100, AlertSeverity::Warning, 1, "ARRIVAL_WAIT", "Arrival waiting", 0, false},
        {101, AlertSeverity::Warning, 2, "ROOM_NOT_READY", "Room not ready", 100, false},
        {102, AlertSeverity::Critical, 3, "HK_BLOCKED", "Housekeeping blocked", 101, false}};
    center.ingest(snapshot);
    center.ingest(snapshot);
    EXPECT_EQ(center.active().size(), static_cast<std::size_t>(3));
    const auto chain = center.causalChain(102);
    EXPECT_EQ(chain.size(), static_cast<std::size_t>(3));
    EXPECT_EQ(chain[0].id, static_cast<std::uint64_t>(100));
    const auto nav = center.navigationFor(102);
    EXPECT_TRUE(nav.has_value());
    EXPECT_EQ(nav->type, UiCommandType::OpenInspector);
    EXPECT_EQ(nav->entityId, static_cast<EntityId>(3));
}

TEST_CASE("Alerts missing from the next authoritative snapshot resolve") {
    AlertCenter center(4);
    center.ingest({{200, AlertSeverity::Warning, 9, "ROOM_DIRTY", "Room needs cleaning", 0, false}});
    EXPECT_EQ(center.active().size(), static_cast<std::size_t>(1));

    center.ingest({});

    EXPECT_TRUE(center.active().empty());
    EXPECT_EQ(center.resolvedHistory().size(), static_cast<std::size_t>(1));
    EXPECT_TRUE(center.resolvedHistory().front().resolved);
    const auto nav = center.navigationFor(200);
    EXPECT_TRUE(nav.has_value());
    EXPECT_EQ(nav->entityId, static_cast<EntityId>(9));
}

TEST_CASE("Resolved alert history remains bounded") {
    AlertCenter center(2);
    for (std::uint64_t i=1;i<=5;++i) {
        center.ingest({{i, AlertSeverity::Info, i, "X", "resolved", 0, true}});
    }
    EXPECT_EQ(center.resolvedHistory().size(), static_cast<std::size_t>(2));
    EXPECT_TRUE(center.causalChain(1).empty());
    EXPECT_FALSE(center.navigationFor(1).has_value());
    EXPECT_FALSE(center.causalChain(5).empty());
}

TEST_CASE("Alert center reconciles thousands of current-state alerts with bounded history") {
    AlertCenter center(200);
    std::vector<AlertSnapshot> first;
    first.reserve(5000);
    for (std::uint64_t i = 1; i <= 5000; ++i)
        first.push_back({i, AlertSeverity::Warning, i, "LOAD", "Operational alert", 0, false});
    center.ingest(first);
    EXPECT_EQ(center.active().size(), static_cast<std::size_t>(5000));

    std::vector<AlertSnapshot> second;
    second.reserve(2500);
    for (std::uint64_t i = 2501; i <= 5000; ++i)
        second.push_back({i, AlertSeverity::Warning, i, "LOAD", "Operational alert", 0, false});
    center.ingest(second);

    EXPECT_EQ(center.active().size(), static_cast<std::size_t>(2500));
    EXPECT_EQ(center.resolvedHistory().size(), static_cast<std::size_t>(200));
    EXPECT_TRUE(center.causalChain(1).empty());
    EXPECT_FALSE(center.navigationFor(1).has_value());
    EXPECT_TRUE(center.navigationFor(5000).has_value());
}
