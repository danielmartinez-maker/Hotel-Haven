#include "TestFramework.h"

#include "hh/frontend/MainMenuView.h"
#include "hh/frontend/MenuHotelProvider.h"

#include <string>

TEST_CASE("property values clamp and missing values use em dash") {
    hh::frontend::MenuPropertySummary summary{};
    summary.hotelName = "The Beaumont";
    summary.city = "Paris";
    summary.country = "France";
    summary.starRating = 8;
    summary.currentDay = 184;
    summary.occupancy = 0.81f;
    summary.guestSatisfaction.reset();
    summary.cashMinorUnits = 128000000;
    summary.currencyCode = "EUR";
    summary.roomCount = 247;

    hh::frontend::MainMenuView view;
    const auto card = view.formatProperty(summary);
    EXPECT_EQ(card.visualStars, 5);
    EXPECT_EQ(card.day, std::string{"184"});
    EXPECT_EQ(card.occupancy, std::string{"81%"});
    EXPECT_EQ(card.satisfaction, std::string{"\xE2\x80\x94"});
    EXPECT_EQ(card.cash, std::string{"\xE2\x82\xAC" "1.28M"});
    EXPECT_EQ(card.rooms, std::string{"247"});
}

TEST_CASE("percent presentation clamps to valid range") {
    hh::frontend::MenuPropertySummary summary{};
    summary.occupancy = 1.5f;
    summary.guestSatisfaction = -0.2f;
    const auto card = hh::frontend::MainMenuView{}.formatProperty(summary);
    EXPECT_EQ(card.occupancy, std::string{"100%"});
    EXPECT_EQ(card.satisfaction, std::string{"0%"});
}

TEST_CASE("ultrawide keeps centered 1920 safe zone") {
    const auto layout = hh::frontend::MainMenuView{}.layout(3840.0f, 1080.0f, 1.0f);
    EXPECT_NEAR(layout.safeZoneLeft, 960.0f, 0.01f);
    EXPECT_NEAR(layout.safeZoneWidth, 1920.0f, 0.01f);
    EXPECT_NEAR(layout.navigationLeft, 1032.0f, 0.01f);
}

TEST_CASE("minimum resolution scales the logical canvas uniformly") {
    const auto layout = hh::frontend::MainMenuView{}.layout(1280.0f, 720.0f, 1.0f);
    EXPECT_NEAR(layout.logicalScale, 2.0f / 3.0f, 0.001f);
    EXPECT_NEAR(layout.navigationLeft, 48.0f, 0.01f);
    EXPECT_NEAR(layout.navigationTop, 200.0f, 0.01f);
}

TEST_CASE("ui scaling preserves world aspect and scales UI coordinates") {
    const auto layout = hh::frontend::MainMenuView{}.layout(1920.0f, 1080.0f, 1.25f);
    EXPECT_NEAR(layout.uiScale, 1.25f, 0.001f);
    EXPECT_NEAR(layout.navigationLeft, 90.0f, 0.01f);
}
