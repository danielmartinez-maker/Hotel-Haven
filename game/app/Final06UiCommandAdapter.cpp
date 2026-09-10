#include "Final06UiCommandAdapter.h"

#include <cstdint>
#include <limits>

namespace hh::client {
namespace {

hh::frontend::UiCommandResult rejected(std::string code, std::string message) {
  return {false, std::move(code), std::move(message)};
}

} // namespace

hh::frontend::UiCommandResult
dispatchFinal06UiCommand(hh::game::SimulationEconomyBridge &authority,
                         const hh::frontend::UiCommand &command) {
  using hh::frontend::UiCommandType;

  if (command.type == UiCommandType::SetFutureRate) {
    if (command.textValue.empty() || command.integerValue <= 0 ||
        command.secondaryIntegerValue < 0 ||
        command.secondaryIntegerValue > std::numeric_limits<int>::max()) {
      return rejected("INVALID_PRICING_RULE",
                      "Future rate requires a category, non-negative day, and positive exact-cent rate");
    }

    const auto snapshot = authority.revenueManagementSnapshot();
    std::uint64_t nextRuleId = 1;
    for (const auto &rule : snapshot.rules) {
      if (rule.ruleId == std::numeric_limits<std::uint64_t>::max())
        return rejected("PRICING_RULE_ID_EXHAUSTED",
                        "No pricing-rule identifier remains available");
      if (rule.ruleId >= nextRuleId)
        nextRuleId = rule.ruleId + 1;
    }

    hh::game::PricingRuleCommand rule;
    rule.ruleId = nextRuleId;
    rule.startDay = static_cast<int>(command.secondaryIntegerValue);
    rule.endDay = rule.startDay;
    rule.weekdayMask = 0x7f;
    rule.roomCategory = command.textValue;
    rule.minOccupancyBasisPoints = 0;
    rule.maxOccupancyBasisPoints = 10000;
    rule.rateCents = command.integerValue;
    rule.priority = 0;
    rule.minRateCents = command.integerValue;
    rule.maxRateCents = command.integerValue;
    const auto result = authority.setPricingRule(rule);
    return {result.ok, result.ok ? std::string{} : result.reason,
            result.ok ? "Future rate rule applied" : result.reason};
  }

  if (command.type == UiCommandType::SetOverbookingPolicy) {
    if (command.integerValue < 0 ||
        command.integerValue > std::numeric_limits<int>::max()) {
      return rejected("INVALID_OVERBOOKING_POLICY",
                      "Overbooking allowance must be a non-negative integer");
    }

    hh::game::OverbookingPolicy policy;
    const auto &policies = authority.overbookingPolicies();
    const auto current = policies.find("standard");
    if (current != policies.end()) {
      policy = current->second;
    } else {
      policy.roomCategory = "standard";
      policy.startDay = authority.view().day;
      policy.endDay = std::numeric_limits<int>::max();
    }
    policy.allowance = static_cast<int>(command.integerValue);
    const auto result = authority.setOverbookingPolicy(policy);
    return {result.ok, result.ok ? std::string{} : result.reason,
            result.ok ? "Overbooking allowance applied" : result.reason};
  }

  if (command.type == UiCommandType::StartMarketingCampaign) {
    return rejected(
        "FINAL06_CAMPAIGN_OFFER_UNAVAILABLE",
        "FINAL-06 does not expose an authoritative campaign-offer catalog to the playable application yet");
  }

  if (command.type == UiCommandType::AcceptContract) {
    return rejected(
        "FINAL06_CONTRACT_OFFER_UNAVAILABLE",
        "FINAL-06 does not expose an authoritative contract-offer catalog to the playable application yet");
  }

  return rejected("FINAL06_UI_COMMAND_UNSUPPORTED",
                  "Command is not owned by the FINAL-06 application adapter");
}

} // namespace hh::client
