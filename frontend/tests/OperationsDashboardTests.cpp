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

TEST_CASE("Filtered operations lists virtualize before materializing rows") {
    OperationsSnapshot source;
    for (std::uint64_t i = 0; i < 10000; ++i) {
        const auto area = (i % 3 == 0) ? OperationArea::Engineering
                                       : OperationArea::Housekeeping;
        source.rows.push_back({i, area, "Task", "Ready", 1, "", 0});
    }

    OperationsDashboard dashboard;
    dashboard.update(source);
    const auto page = dashboard.filteredWindow(OperationArea::Engineering, 1000, 64);

    EXPECT_EQ(page.size(), static_cast<std::size_t>(64));
    EXPECT_EQ(page.front().id, static_cast<EntityId>(3000));
    EXPECT_EQ(page.back().id, static_cast<EntityId>(3189));
}

TEST_CASE("Zero-sized filtered operations windows materialize no rows") {
    OperationsSnapshot source;
    source.rows = {
        {1, OperationArea::Housekeeping, "Room 1 turnover", "Ready", 1, "", 0},
        {2, OperationArea::Housekeeping, "Room 2 turnover", "Ready", 1, "", 0},
        {3, OperationArea::Engineering, "Boiler PM", "Ready", 1, "", 0}
    };

    OperationsDashboard dashboard;
    dashboard.update(source);

    const auto page = dashboard.filteredWindow(OperationArea::Housekeeping, 0, 0);
    EXPECT_TRUE(page.empty());
}
