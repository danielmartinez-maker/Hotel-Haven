#include "TestFramework.h"

#include "hh/frontend/MenuSceneController.h"

#include <cmath>

TEST_CASE("reduced motion zeros camera offsets") {
    hh::frontend::MenuSceneController scene;
    scene.setReducedMotion(true);
    const auto pose = scene.cameraPose(8.0F, {});
    EXPECT_NEAR(pose.yawOffsetDegrees, 0.0F, 0.0001F);
    EXPECT_NEAR(pose.targetXOffset, 0.0F, 0.0001F);
    EXPECT_NEAR(pose.targetZOffset, 0.0F, 0.0001F);
    EXPECT_NEAR(pose.zoomScale, 1.0F, 0.0001F);
}

TEST_CASE("idle motion remains within approved bounds") {
    hh::frontend::MenuSceneController scene;
    for (int i = 0; i < 1000; ++i) {
        const auto pose = scene.cameraPose(static_cast<float>(i), {});
        EXPECT_TRUE(std::fabs(pose.idleYawDegrees) <= 2.001F);
        EXPECT_TRUE(pose.idleZoomScale >= 0.969F);
        EXPECT_TRUE(pose.idleZoomScale <= 1.031F);
    }
}

TEST_CASE("settings selection almost stops camera") {
    hh::frontend::MenuSceneController scene;
    hh::frontend::MenuTransitionState state{};
    state.targetItem = hh::frontend::MainMenuItem::Settings;
    state.easedProgress = 1.0F;
    state.complete = true;
    const auto pose = scene.cameraPose(17.0F, state);
    EXPECT_NEAR(pose.idleYawDegrees, 0.0F, 0.0001F);
    EXPECT_NEAR(pose.idleZoomScale, 1.0F, 0.0001F);
}

TEST_CASE("ambient scheduler is deterministic for a fixed seed") {
    hh::frontend::AmbientPresentationScheduler first(42U);
    hh::frontend::AmbientPresentationScheduler second(42U);
    EXPECT_EQ(first.nextEvent(), second.nextEvent());
    EXPECT_EQ(first.nextEvent(), second.nextEvent());
    EXPECT_EQ(first.nextEvent(), second.nextEvent());
}

TEST_CASE("ambient major event cadence stays between five and fifteen seconds") {
    hh::frontend::AmbientPresentationScheduler scheduler(7U);
    for (int i = 0; i < 100; ++i) {
        const auto event = scheduler.nextEvent();
        EXPECT_TRUE(event.secondsUntilNext >= 5.0F);
        EXPECT_TRUE(event.secondsUntilNext <= 15.0F);
    }
}
