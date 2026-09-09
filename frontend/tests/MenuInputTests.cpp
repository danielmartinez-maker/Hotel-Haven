#include "TestFramework.h"

#include "hh/frontend/MenuInput.h"

TEST_CASE("gamepad mapper emits edge triggered menu actions") {
    hh::frontend::MenuGamepadMapper mapper;
    hh::frontend::MenuGamepadState state{};
    state.connected = true;
    state.dpadDown = true;
    state.a = true;

    const auto first = mapper.update(state);
    EXPECT_EQ(first.navigationDelta, 1);
    EXPECT_TRUE(first.activate);
    EXPECT_FALSE(first.cancel);

    const auto held = mapper.update(state);
    EXPECT_EQ(held.navigationDelta, 0);
    EXPECT_FALSE(held.activate);

    state.dpadDown = false;
    state.a = false;
    state.b = true;
    const auto cancel = mapper.update(state);
    EXPECT_TRUE(cancel.cancel);
}

TEST_CASE("gamepad mapper supports dpad and left stick navigation") {
    hh::frontend::MenuGamepadMapper mapper;
    hh::frontend::MenuGamepadState state{};
    state.connected = true;
    state.dpadUp = true;
    EXPECT_EQ(mapper.update(state).navigationDelta, -1);

    state = {};
    state.connected = true;
    static_cast<void>(mapper.update(state));
    state.leftStickY = -20000;
    EXPECT_EQ(mapper.update(state).navigationDelta, 1);
}

TEST_CASE("disconnected gamepad produces no menu actions and clears held edges") {
    hh::frontend::MenuGamepadMapper mapper;
    hh::frontend::MenuGamepadState state{};
    state.connected = true;
    state.a = true;
    EXPECT_TRUE(mapper.update(state).activate);

    state = {};
    const auto disconnected = mapper.update(state);
    EXPECT_EQ(disconnected.navigationDelta, 0);
    EXPECT_FALSE(disconnected.activate);
    EXPECT_FALSE(disconnected.cancel);

    state.connected = true;
    state.a = true;
    EXPECT_TRUE(mapper.update(state).activate);
}
