#include "TestFramework.h"

#include "hh/frontend/MainMenuModel.h"

#include <cstddef>

TEST_CASE("menu order is specification order") {
    const auto items = hh::frontend::MainMenuModel::orderedItems();
    EXPECT_EQ(items.size(), std::size_t{8});
    EXPECT_EQ(items[0], hh::frontend::MainMenuItem::Continue);
    EXPECT_EQ(items[1], hh::frontend::MainMenuItem::NewHotel);
    EXPECT_EQ(items[2], hh::frontend::MainMenuItem::LoadHotel);
    EXPECT_EQ(items[3], hh::frontend::MainMenuItem::Scenarios);
    EXPECT_EQ(items[4], hh::frontend::MainMenuItem::Sandbox);
    EXPECT_EQ(items[5], hh::frontend::MainMenuItem::Settings);
    EXPECT_EQ(items[6], hh::frontend::MainMenuItem::Credits);
    EXPECT_EQ(items[7], hh::frontend::MainMenuItem::Quit);
}

TEST_CASE("no save disables continue and selects new hotel") {
    hh::frontend::MainMenuModel model(false);
    EXPECT_FALSE(model.isEnabled(hh::frontend::MainMenuItem::Continue));
    EXPECT_EQ(model.selected(), hh::frontend::MainMenuItem::NewHotel);
}

TEST_CASE("valid save enables continue and selects continue") {
    hh::frontend::MainMenuModel model(true);
    EXPECT_TRUE(model.isEnabled(hh::frontend::MainMenuItem::Continue));
    EXPECT_EQ(model.selected(), hh::frontend::MainMenuItem::Continue);
}

TEST_CASE("disabled item cannot become selected") {
    hh::frontend::MainMenuModel model(false);
    model.select(hh::frontend::MainMenuItem::Continue);
    EXPECT_EQ(model.selected(), hh::frontend::MainMenuItem::NewHotel);
}
