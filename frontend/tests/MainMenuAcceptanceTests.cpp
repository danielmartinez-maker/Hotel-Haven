#include "TestFramework.h"

#include <array>
#include <utility>

#include "hh/frontend/MainMenuController.h"
#include "hh/frontend/MainMenuView.h"
#include "hh/frontend/MenuSceneController.h"

TEST_CASE("no-save and valid-save initial focus follow specification") {
    hh::frontend::MainMenuModel noSave(false);
    EXPECT_FALSE(noSave.isEnabled(hh::frontend::MainMenuItem::Continue));
    EXPECT_EQ(noSave.selected(), hh::frontend::MainMenuItem::NewHotel);

    hh::frontend::MainMenuModel validSave(true);
    EXPECT_TRUE(validSave.isEnabled(hh::frontend::MainMenuItem::Continue));
    EXPECT_EQ(validSave.selected(), hh::frontend::MainMenuItem::Continue);
}

TEST_CASE("layout remains inside supported resolution matrix") {
    hh::frontend::MainMenuView view;
    constexpr std::array<std::pair<float, float>, 5> sizes{{
        {1280.0F, 720.0F},
        {1920.0F, 1080.0F},
        {2560.0F, 1080.0F},
        {3840.0F, 1080.0F},
        {1600.0F, 1200.0F},
    }};

    for (const auto& [width, height] : sizes) {
        const auto layout = view.layout(width, height, 1.0F);
        EXPECT_TRUE(layout.logicalScale > 0.0F);
        EXPECT_TRUE(layout.navigationLeft >= 0.0F);
        EXPECT_TRUE(layout.navigationTop >= 0.0F);
        EXPECT_TRUE(layout.navigationLeft + layout.navigationWidth <= width + 0.01F);
        EXPECT_TRUE(layout.propertyCardLeft >= 0.0F);
        EXPECT_TRUE(layout.propertyCardLeft + layout.propertyCardWidth <= width + 0.01F);
        EXPECT_TRUE(layout.versionBottom <= height + 0.01F);
    }
}

TEST_CASE("quit cancellation preserves selected item and restores navigation") {
    hh::frontend::MainMenuModel model(true);
    hh::frontend::MainMenuController controller(model);
    EXPECT_TRUE(model.select(hh::frontend::MainMenuItem::Quit));
    EXPECT_EQ(controller.activate(), hh::frontend::MainMenuCommand::None);
    EXPECT_EQ(model.modal(), hh::frontend::MainMenuModal::QuitConfirm);
    controller.navigate(1);
    EXPECT_EQ(model.selected(), hh::frontend::MainMenuItem::Quit);
    EXPECT_TRUE(controller.cancel());
    controller.navigate(1);
    EXPECT_EQ(model.selected(), hh::frontend::MainMenuItem::Continue);
}

TEST_CASE("reduced motion removes idle and selection camera motion") {
    hh::frontend::MenuSceneController scene;
    scene.setReducedMotion(true);
    hh::frontend::MenuTransitionState state{};
    state.targetItem = hh::frontend::MainMenuItem::Sandbox;
    state.easedProgress = 1.0F;
    state.reducedMotion = true;
    const auto pose = scene.cameraPose(123.0F, state);
    EXPECT_NEAR(pose.idleYawDegrees, 0.0F, 0.0001F);
    EXPECT_NEAR(pose.idleZoomScale, 1.0F, 0.0001F);
    EXPECT_NEAR(pose.yawOffsetDegrees, 0.0F, 0.0001F);
    EXPECT_NEAR(pose.targetXOffset, 0.0F, 0.0001F);
    EXPECT_NEAR(pose.targetZOffset, 0.0F, 0.0001F);
    EXPECT_NEAR(pose.zoomScale, 1.0F, 0.0001F);
}
