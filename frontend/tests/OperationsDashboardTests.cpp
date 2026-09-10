#include "TestFramework.h"
#include "hh/frontend/OperationsDashboard.h"

using namespace hh::frontend;

TEST_CASE("Operations dashboard filters without changing authoritative scheduler order") {
    OperationsSnapshot source;
    source.rows = {
        {9, OperationArea::Housekeeping, "Room 9 turnover", "Blocked", 4, "HK_NO_LINEN", 900},
        {3, OperationArea::Engineering, "Boiler PM", "Ready", 2, "", 450},
        {5, OperationArea::Housekeeping, "Room 5 turnover", "Working", 1, "", 120}
    };
    OperationsDashboard dashboard;
    dashboard.update(source);
    const auto hk = dashboard.filtered(OperationArea::Housekeeping);
    EXPECT_EQ(hk.size(), static_cast<std::size_t>(2));
    EXPECT_EQ(hk[0].id, static_cast<EntityId>(9));
    EXPECT_EQ(hk[1].id, static_cast<EntityId>(5));
    EXPECT_EQ(source.rows[0].id, static_cast<EntityId>(9));
}

TEST_CASE("Large operations lists expose bounded virtual windows") {
    OperationsSnapshot source;
    for (std::uint64_t i=0;i<5000;++i)
        source.rows.push_back({i, OperationArea::Housekeeping, "Task", "Ready", 1, "", 0});
    OperationsDashboard dashboard;
    dashboard.update(source);
    const auto window = dashboard.window(1200, 80);
    EXPECT_EQ(window.size(), static_cast<std::size_t>(80));
    EXPECT_EQ(window.front().id, static_cast<EntityId>(1200));
}
