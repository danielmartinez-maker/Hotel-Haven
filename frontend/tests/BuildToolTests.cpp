#include "TestFramework.h"
#include "hh/frontend/BuildCatalog.h"

using namespace hh::frontend;

TEST_CASE("Build tool honors authoritative rejection verbatim") {
    BuildToolController controller;
    BuildPlacementPreview preview;
    preview.requestId = 41;
    preview.itemId = "guest_room";
    preview.valid = false;
    preview.reasonCode = "BUILD_FIRE_EGRESS_BLOCKED";
    preview.reasonText = "Placement blocks required fire egress";
    preview.costCents = 125000;
    controller.applyAuthoritativePreview(preview);
    EXPECT_FALSE(controller.canConfirm());
    EXPECT_EQ(controller.rejectionCode(), std::string("BUILD_FIRE_EGRESS_BLOCKED"));
    EXPECT_FALSE(controller.confirmCommand().has_value());
}

TEST_CASE("Build catalog searches and filters presentation metadata") {
    BuildCatalog catalog({{"guest_room","Guest Room","Rooms",125000,{"bed","bath"},90},
                          {"service_elevator","Service Elevator","Infrastructure",900000,{"steel"},480}});
    const auto rows = catalog.filter("elevator", "Infrastructure");
    EXPECT_EQ(rows.size(), static_cast<std::size_t>(1));
    EXPECT_EQ(rows[0].id, std::string("service_elevator"));
}

TEST_CASE("Changing build selection invalidates a preview for the previous item") {
    BuildToolController controller;
    controller.selectItem("guest_room");

    BuildPlacementPreview preview;
    preview.requestId = 42;
    preview.itemId = "guest_room";
    preview.valid = true;
    controller.applyAuthoritativePreview(preview);
    EXPECT_TRUE(controller.canConfirm());

    controller.selectItem("service_elevator");
    EXPECT_FALSE(controller.canConfirm());
    EXPECT_FALSE(controller.confirmCommand().has_value());
}

TEST_CASE("Late authoritative preview for an old build selection cannot replace current preview") {
    BuildToolController controller;
    controller.selectItem("guest_room");

    BuildPlacementPreview stale;
    stale.requestId = 43;
    stale.itemId = "service_elevator";
    stale.valid = true;
    controller.applyAuthoritativePreview(stale);

    EXPECT_FALSE(controller.canConfirm());
    EXPECT_FALSE(controller.confirmCommand().has_value());
    EXPECT_TRUE(controller.preview().itemId.empty());
}
