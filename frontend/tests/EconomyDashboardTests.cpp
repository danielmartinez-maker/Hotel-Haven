#include "TestFramework.h"
#include "hh/frontend/EconomyDashboard.h"
#include <limits>

using namespace hh::frontend;

TEST_CASE("Economy dashboard formats exact cents without floating conversion") {
    EXPECT_EQ(EconomyDashboard::formatMoney(123456), std::string("$1,234.56"));
    EXPECT_EQ(EconomyDashboard::formatMoney(-7), std::string("-$0.07"));
    EXPECT_EQ(EconomyDashboard::formatMoney(5), std::string("$0.05"));
    EXPECT_EQ(EconomyDashboard::formatMoney(std::numeric_limits<std::int64_t>::min()),
              std::string("-$92,233,720,368,547,758.08"));
}

TEST_CASE("Revenue controls emit typed FINAL-06 commands") {
    EconomyDashboard dashboard;
    const auto command = dashboard.setFutureRateCommand(21, "deluxe", 18999);
    EXPECT_EQ(command.type, UiCommandType::SetFutureRate);
    EXPECT_EQ(command.integerValue, static_cast<std::int64_t>(18999));
    EXPECT_EQ(command.secondaryIntegerValue, static_cast<std::int64_t>(21));
    EXPECT_EQ(command.textValue, std::string("deluxe"));
    dashboard.applyCommandResult({false, "RATE_RULE_CONTRACTED", "Contracted rate is protected"});
    EXPECT_EQ(dashboard.lastRejectionCode(), std::string("RATE_RULE_CONTRACTED"));
}

TEST_CASE("Finance adjustments derive only from authoritative rule snapshots") {
    EconomySnapshot snapshot;
    snapshot.pricingRules.push_back({7, 21, 23, "deluxe", 18'999});
    snapshot.overbookingPolicies.push_back({"standard", 2, 45'000, 20, 40});

    EconomyDashboard dashboard;
    dashboard.update(snapshot);

    const auto rate = dashboard.adjustPricingRuleCommand(0, 1'000);
    EXPECT_TRUE(rate.has_value());
    EXPECT_EQ(rate->type, UiCommandType::SetFutureRate);
    EXPECT_EQ(rate->integerValue, static_cast<std::int64_t>(19'999));
    EXPECT_EQ(rate->secondaryIntegerValue, static_cast<std::int64_t>(21));
    EXPECT_EQ(rate->textValue, std::string("deluxe"));

    const auto overbooking = dashboard.adjustOverbookingCommand(0, -1);
    EXPECT_TRUE(overbooking.has_value());
    EXPECT_EQ(overbooking->type, UiCommandType::SetOverbookingPolicy);
    EXPECT_EQ(overbooking->integerValue, static_cast<std::int64_t>(1));
}

TEST_CASE("Finance adjustments reject invalid indexes and numeric boundaries") {
    EconomySnapshot snapshot;
    snapshot.pricingRules.push_back(
        {8, 4, 4, "suite", std::numeric_limits<std::int64_t>::max()});
    snapshot.overbookingPolicies.push_back({"standard", 0, 0, 0, 10});
    snapshot.overbookingPolicies.push_back({"suite", 1, 0, 0, 10});

    EconomyDashboard dashboard;
    dashboard.update(snapshot);

    EXPECT_FALSE(dashboard.adjustPricingRuleCommand(1, 100).has_value());
    EXPECT_FALSE(dashboard.adjustPricingRuleCommand(0, 1).has_value());
    EXPECT_FALSE(dashboard.adjustPricingRuleCommand(0,
                 -std::numeric_limits<std::int64_t>::max()).has_value());
    EXPECT_FALSE(dashboard.adjustOverbookingCommand(2, 1).has_value());
    EXPECT_FALSE(dashboard.adjustOverbookingCommand(0, -1).has_value());
    EXPECT_FALSE(dashboard.adjustOverbookingCommand(1, 1).has_value());
}
