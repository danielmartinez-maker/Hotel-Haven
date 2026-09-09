#include "TestFramework.h"

#include "hh/frontend/MenuTransitionDirector.h"

#include <cstddef>

TEST_CASE("retargeting interrupts instead of queueing") {
    hh::frontend::MenuTransitionDirector director;
    director.retarget(hh::frontend::MainMenuItem::LoadHotel, false);
    static_cast<void>(director.update(0.20F));
    director.retarget(hh::frontend::MainMenuItem::Scenarios, false);
    EXPECT_EQ(director.targetItem(), hh::frontend::MainMenuItem::Scenarios);
    EXPECT_EQ(director.queuedTransitionCount(), std::size_t{0});
}

TEST_CASE("reduced motion uses short opacity transition") {
    hh::frontend::MenuTransitionDirector director;
    director.retarget(hh::frontend::MainMenuItem::NewHotel, true);
    const auto state = director.update(0.20F);
    EXPECT_TRUE(state.complete);
    EXPECT_NEAR(state.itemShiftPixels, 0.0F, 0.001F);
    EXPECT_NEAR(state.focusOpacity, 1.0F, 0.001F);
}

TEST_CASE("normal focus transition shifts text subtly") {
    hh::frontend::MenuTransitionDirector director;
    director.retarget(hh::frontend::MainMenuItem::NewHotel, false);
    const auto state = director.update(1.0F);
    EXPECT_TRUE(state.complete);
    EXPECT_TRUE(state.itemShiftPixels >= 8.0F);
    EXPECT_TRUE(state.itemShiftPixels <= 12.0F);
}
