#include "TestFramework.h"
#include "hh/frontend/ObjectiveUi.h"

using namespace hh::frontend;

TEST_CASE("Objective UI reflects authoritative progress and reason codes") {
    ObjectiveUi ui;
    ui.update({{{7, "Open 10 rooms", 6, 10, false, "OBJ_NEEDS_ROOMS", "4 more sellable rooms required", true}}});
    const auto& item = ui.items().front();
    EXPECT_EQ(item.current, static_cast<std::int64_t>(6));
    EXPECT_EQ(item.target, static_cast<std::int64_t>(10));
    EXPECT_FALSE(item.complete);
    EXPECT_EQ(item.reasonCode, std::string("OBJ_NEEDS_ROOMS"));
    EXPECT_TRUE(item.dismissible);
    EXPECT_TRUE(ui.dismiss(7));
}
