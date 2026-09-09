#include "TestFramework.h"

#include "hh/frontend/MainMenuController.h"

TEST_CASE("navigation wraps and skips disabled continue") {
    hh::frontend::MainMenuModel model(false);
    hh::frontend::MainMenuController controller(model);
    controller.navigate(-1);
    EXPECT_EQ(model.selected(), hh::frontend::MainMenuItem::Quit);
    controller.navigate(1);
    EXPECT_EQ(model.selected(), hh::frontend::MainMenuItem::NewHotel);
}

TEST_CASE("activation maps menu items to application commands") {
    hh::frontend::MainMenuModel model(true);
    hh::frontend::MainMenuController controller(model);

    EXPECT_TRUE(model.select(hh::frontend::MainMenuItem::Continue));
    EXPECT_EQ(controller.activate(), hh::frontend::MainMenuCommand::ContinueLatest);

    EXPECT_TRUE(model.select(hh::frontend::MainMenuItem::NewHotel));
    EXPECT_EQ(controller.activate(), hh::frontend::MainMenuCommand::StartNewHotel);

    EXPECT_TRUE(model.select(hh::frontend::MainMenuItem::LoadHotel));
    EXPECT_EQ(controller.activate(), hh::frontend::MainMenuCommand::OpenLoadHotel);
}

TEST_CASE("settings and credits activate as living scene overlays") {
    hh::frontend::MainMenuModel model(true);
    hh::frontend::MainMenuController controller(model);

    EXPECT_TRUE(model.select(hh::frontend::MainMenuItem::Settings));
    EXPECT_EQ(controller.activate(), hh::frontend::MainMenuCommand::None);
    EXPECT_EQ(model.panel(), hh::frontend::MainMenuPanel::Settings);
    EXPECT_TRUE(controller.cancel());
    EXPECT_EQ(model.panel(), hh::frontend::MainMenuPanel::None);

    EXPECT_TRUE(model.select(hh::frontend::MainMenuItem::Credits));
    EXPECT_EQ(controller.activate(), hh::frontend::MainMenuCommand::None);
    EXPECT_EQ(model.panel(), hh::frontend::MainMenuPanel::Credits);
}

TEST_CASE("navigation is suspended while a submenu panel is open") {
    hh::frontend::MainMenuModel model(true);
    hh::frontend::MainMenuController controller(model);
    EXPECT_TRUE(model.select(hh::frontend::MainMenuItem::Settings));
    EXPECT_EQ(controller.activate(), hh::frontend::MainMenuCommand::None);
    controller.navigate(1);
    EXPECT_EQ(model.selected(), hh::frontend::MainMenuItem::Settings);
}

TEST_CASE("quit activation opens modal before emitting exit") {
    hh::frontend::MainMenuModel model(true);
    hh::frontend::MainMenuController controller(model);
    EXPECT_TRUE(model.select(hh::frontend::MainMenuItem::Quit));
    EXPECT_EQ(controller.activate(), hh::frontend::MainMenuCommand::None);
    EXPECT_EQ(model.modal(), hh::frontend::MainMenuModal::QuitConfirm);
    EXPECT_EQ(controller.confirmQuit(), hh::frontend::MainMenuCommand::ExitApplication);
}

TEST_CASE("cancel closes quit modal") {
    hh::frontend::MainMenuModel model(true);
    hh::frontend::MainMenuController controller(model);
    EXPECT_TRUE(model.select(hh::frontend::MainMenuItem::Quit));
    EXPECT_EQ(controller.activate(), hh::frontend::MainMenuCommand::None);
    EXPECT_TRUE(controller.cancel());
    EXPECT_EQ(model.modal(), hh::frontend::MainMenuModal::None);
}
