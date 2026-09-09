#include "TestFramework.h"

#include "hh/frontend/MainMenuController.h"

TEST_CASE("ten thousand navigation events preserve valid focus") {
    hh::frontend::MainMenuModel model(false);
    hh::frontend::MainMenuController controller(model);
    for (int i = 0; i < 10000; ++i) {
        controller.navigate((i % 3) == 0 ? -1 : 1);
        EXPECT_TRUE(model.isEnabled(model.selected()));
        EXPECT_FALSE(model.selected() == hh::frontend::MainMenuItem::Continue);
    }
}

TEST_CASE("rapid quit modal open close does not corrupt focus") {
    hh::frontend::MainMenuModel model(true);
    hh::frontend::MainMenuController controller(model);
    for (int i = 0; i < 1000; ++i) {
        EXPECT_TRUE(model.select(hh::frontend::MainMenuItem::Quit));
        EXPECT_EQ(controller.activate(), hh::frontend::MainMenuCommand::None);
        EXPECT_EQ(model.modal(), hh::frontend::MainMenuModal::QuitConfirm);
        EXPECT_TRUE(controller.cancel());
        EXPECT_EQ(model.modal(), hh::frontend::MainMenuModal::None);
        EXPECT_EQ(model.selected(), hh::frontend::MainMenuItem::Quit);
    }
}
