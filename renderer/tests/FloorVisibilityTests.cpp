#include "TestFramework.h"
#include "hh/renderer/FloorVisibility.h"

TEST_CASE("normal floor mode shows only active floor") {
    using namespace hh::renderer;
    EXPECT_EQ(floorVisibility(1, 1, FloorContextMode::Normal), FloorRenderVisibility::Full);
    EXPECT_EQ(floorVisibility(0, 1, FloorContextMode::Normal), FloorRenderVisibility::Hidden);
    EXPECT_EQ(floorVisibility(2, 1, FloorContextMode::Normal), FloorRenderVisibility::Hidden);
}

TEST_CASE("context mode shows only immediately adjacent floors as shells") {
    using namespace hh::renderer;
    EXPECT_EQ(floorVisibility(1, 1, FloorContextMode::AdjacentContext), FloorRenderVisibility::Full);
    EXPECT_EQ(floorVisibility(0, 1, FloorContextMode::AdjacentContext), FloorRenderVisibility::TranslucentShell);
    EXPECT_EQ(floorVisibility(2, 1, FloorContextMode::AdjacentContext), FloorRenderVisibility::TranslucentShell);
    EXPECT_EQ(floorVisibility(-1, 1, FloorContextMode::AdjacentContext), FloorRenderVisibility::Hidden);
    EXPECT_EQ(floorVisibility(3, 1, FloorContextMode::AdjacentContext), FloorRenderVisibility::Hidden);
}
