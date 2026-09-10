#include "TestFramework.h"
#include "hh/frontend/InspectorModel.h"

using namespace hh::frontend;

TEST_CASE("Inspector preserves authoritative causal diagnostics") {
    UiEntitySnapshot entity;
    entity.id = 77;
    entity.kind = InspectorKind::Task;
    entity.title = "Room 412 turnover";
    entity.fields.push_back({"Status", "Blocked"});
    entity.diagnostics.push_back({"HK_NO_CLEAN_LINEN", "Clean linen unavailable", 9001, 0});
    const auto model = InspectorModel::compose(entity);
    EXPECT_EQ(model.entityId, static_cast<EntityId>(77));
    EXPECT_EQ(model.diagnostics.size(), static_cast<std::size_t>(1));
    EXPECT_EQ(model.diagnostics[0].code, std::string("HK_NO_CLEAN_LINEN"));
    EXPECT_EQ(model.diagnostics[0].sourceEntityId, static_cast<EntityId>(9001));
    EXPECT_EQ(model.diagnostics[0].message, std::string("Clean linen unavailable"));
}

TEST_CASE("Inspector does not invent causal diagnostics") {
    UiEntitySnapshot entity;
    entity.id = 11;
    entity.kind = InspectorKind::Room;
    entity.title = "Room 11";
    const auto model = InspectorModel::compose(entity);
    EXPECT_TRUE(model.diagnostics.empty());
}
