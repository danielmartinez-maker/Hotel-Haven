#include "TestFramework.h"
#include "hh/frontend/GameHudController.h"
#include "hh/frontend/GameHudModel.h"

using namespace hh::frontend;

TEST_CASE("HUD binds authoritative snapshot fields without simulation logic") {
    SimulationSnapshot source;
    source.revision = 4;
    source.hud.cashCents = 123456;
    source.hud.day = 12;
    source.hud.hour = 14;
    source.hud.minute = 7;
    source.hud.speed = SimulationSpeed::FourX;
    source.hud.occupancyPermille = 835;
    source.hud.satisfactionPermille = 912;
    source.hud.reputationPermille = 876;
    source.hud.alertCount = 3;
    source.hud.currentTool = "Inspect";
    source.hud.activeFloor = 2;
    source.hud.cutaway = true;
    GameHudModel model;
    model.update(source.hud);
    EXPECT_EQ(model.cashText(), std::string("$1,234.56"));
    EXPECT_EQ(model.timeText(), std::string("Day 13 14:07"));
    EXPECT_EQ(model.speed(), SimulationSpeed::FourX);
    EXPECT_EQ(model.occupancyText(), std::string("83.5%"));
    EXPECT_EQ(model.alertCount(), 3);
    EXPECT_EQ(model.activeFloor(), 2);
    EXPECT_TRUE(model.cutaway());
}

TEST_CASE("HUD input routes speed and focus as typed commands") {
    GameHudModel model;
    GameHudController controller(model);
    controller.setFocus(HudFocus::Speed);
    const auto speed = controller.handleAction(UiAction::Activate);
    EXPECT_TRUE(speed.has_value());
    EXPECT_EQ(speed->type, UiCommandType::SetSimulationSpeed);
    EXPECT_EQ(speed->integerValue, static_cast<std::int64_t>(SimulationSpeed::OneX));
    controller.setFocus(HudFocus::Alerts);
    const auto alerts = controller.handleAction(UiAction::Activate);
    EXPECT_TRUE(alerts.has_value());
    EXPECT_EQ(alerts->type, UiCommandType::OpenManagementPanel);
    EXPECT_EQ(alerts->integerValue, static_cast<std::int64_t>(ManagementPanelId::Alerts));
}
