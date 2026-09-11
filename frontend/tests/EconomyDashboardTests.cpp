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
