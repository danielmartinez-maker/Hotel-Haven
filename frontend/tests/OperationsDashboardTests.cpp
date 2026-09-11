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
    EXPECT_EQ(dashboard.filteredCount(OperationArea::Engineering),
              static_cast<std::size_t>(3334));
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

TEST_CASE("Operations filter matrix uses only authoritative row fields") {
    OperationsSnapshot source;
    source.rows = {
        {11, OperationArea::Housekeeping, "Room turnover", "Blocked", 4,
         "HK_NO_LINEN", 0, 501, 101, 2, 7, 9},
        {12, OperationArea::Housekeeping, "Room turnover", "Working", 2,
         "", 0, 502, 102, 2, 8, 9},
        {13, OperationArea::Engineering, "Repair", "Blocked", 5,
         "AWAITING_PART", 0, 501, 103, 1, 7, 9}
    };
    OperationsDashboard dashboard;
    dashboard.update(source);

    OperationsFilter filter;
    filter.area = OperationArea::Housekeeping;
    filter.taskType = "Room turnover";
    filter.minimumPriority = 3;
    filter.state = "Blocked";
    filter.reasonCode = "HK_NO_LINEN";
    filter.assigneeId = 501;
    filter.floor = 2;
    filter.x = 7;
    filter.y = 9;

    EXPECT_EQ(dashboard.filteredCount(filter), static_cast<std::size_t>(1));
    const auto page = dashboard.filteredWindow(filter, 0, 8);
    EXPECT_EQ(page.size(), static_cast<std::size_t>(1));
    EXPECT_EQ(page.front().id, static_cast<EntityId>(11));
    EXPECT_EQ(source.rows[0].id, static_cast<EntityId>(11));
}

TEST_CASE("Operations sorting is stable and never mutates scheduler order") {
    OperationsSnapshot source;
    source.rows = {
        {1, OperationArea::Housekeeping, "Task", "Ready", 2, "", 500},
        {2, OperationArea::Housekeeping, "Task", "Ready", 5, "", 100},
        {3, OperationArea::Housekeeping, "Task", "Ready", 5, "", 300},
        {4, OperationArea::Housekeeping, "Task", "Ready", 1, "", 900}
    };
    OperationsDashboard dashboard;
    dashboard.update(source);
    OperationsFilter filter;
    filter.area = OperationArea::Housekeeping;

    const auto priority = dashboard.filteredWindow(
        filter, OperationSort::PriorityHighFirst, 0, 4);
    EXPECT_EQ(priority.size(), static_cast<std::size_t>(4));
    EXPECT_EQ(priority[0].id, static_cast<EntityId>(2));
    EXPECT_EQ(priority[1].id, static_cast<EntityId>(3));
    EXPECT_EQ(priority[2].id, static_cast<EntityId>(1));
    EXPECT_EQ(priority[3].id, static_cast<EntityId>(4));

    const auto oldest = dashboard.filteredWindow(
        filter, OperationSort::OldestFirst, 1, 2);
    EXPECT_EQ(oldest.size(), static_cast<std::size_t>(2));
    EXPECT_EQ(oldest[0].id, static_cast<EntityId>(1));
    EXPECT_EQ(oldest[1].id, static_cast<EntityId>(3));

    EXPECT_EQ(dashboard.snapshot().rows[0].id, static_cast<EntityId>(1));
    EXPECT_EQ(dashboard.snapshot().rows[1].id, static_cast<EntityId>(2));
}

TEST_CASE("Sorted large operations windows remain bounded") {
    OperationsSnapshot source;
    for (std::uint64_t i = 0; i < 10000; ++i) {
        source.rows.push_back({i, OperationArea::Engineering, "Repair", "Ready",
                               static_cast<int>(i % 7), "",
                               static_cast<std::int64_t>(i)});
    }
    OperationsDashboard dashboard;
    dashboard.update(source);
    OperationsFilter filter;
    filter.area = OperationArea::Engineering;

    const auto page = dashboard.filteredWindow(
        filter, OperationSort::OldestFirst, 1000, 64);
    EXPECT_EQ(page.size(), static_cast<std::size_t>(64));
    EXPECT_EQ(page.front().id, static_cast<EntityId>(8999));
    EXPECT_EQ(page.back().id, static_cast<EntityId>(8936));
}
