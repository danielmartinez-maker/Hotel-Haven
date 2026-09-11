#include "hh/frontend/EconomyDashboard.h"
#include <iomanip>
#include <limits>
#include <sstream>
#include <utility>
namespace hh::frontend {
std::string EconomyDashboard::formatMoney(std::int64_t cents) { const bool negative = cents < 0; const std::uint64_t absolute = negative ? static_cast<std::uint64_t>(-(cents + 1)) + 1U : static_cast<std::uint64_t>(cents), dollars = absolute / 100, remainder = absolute % 100; std::string digits = std::to_string(dollars); for (std::ptrdiff_t i = static_cast<std::ptrdiff_t>(digits.size()) - 3; i > 0; i -= 3) digits.insert(static_cast<std::size_t>(i), ","); std::ostringstream out; if (negative) out << '-'; out << '$' << digits << '.' << std::setw(2) << std::setfill('0') << remainder; return out.str(); }
UiCommand EconomyDashboard::setFutureRateCommand(int day, std::string roomCategory, std::int64_t rateCents) const { return {UiCommandType::SetFutureRate, 0, rateCents, day, std::move(roomCategory)}; }
UiCommand EconomyDashboard::setOverbookingCommand(int limit) const { return {UiCommandType::SetOverbookingPolicy, 0, limit}; }
std::optional<UiCommand> EconomyDashboard::adjustPricingRuleCommand(std::size_t index, std::int64_t deltaCents) const {
    if (index >= snapshot_.pricingRules.size()) return std::nullopt;
    const auto& rule = snapshot_.pricingRules[index];
    if (rule.roomCategory.empty() || rule.startDay < 0) return std::nullopt;
    if ((deltaCents > 0 && rule.rateCents > std::numeric_limits<std::int64_t>::max() - deltaCents) ||
        (deltaCents < 0 && rule.rateCents < std::numeric_limits<std::int64_t>::min() - deltaCents)) return std::nullopt;
    const auto rate = rule.rateCents + deltaCents;
    if (rate <= 0) return std::nullopt;
    return setFutureRateCommand(rule.startDay, rule.roomCategory, rate);
}
std::optional<UiCommand> EconomyDashboard::adjustOverbookingCommand(std::size_t index, int delta) const {
    if (index >= snapshot_.overbookingPolicies.size()) return std::nullopt;
    const auto& policy = snapshot_.overbookingPolicies[index];
    if (policy.roomCategory != "standard") return std::nullopt;
    const auto allowance = policy.allowance;
    if ((delta > 0 && allowance > std::numeric_limits<int>::max() - delta) ||
        (delta < 0 && allowance < std::numeric_limits<int>::min() - delta)) return std::nullopt;
    const int adjusted = allowance + delta;
    if (adjusted < 0) return std::nullopt;
    return setOverbookingCommand(adjusted);
}
UiCommand EconomyDashboard::startCampaignCommand(std::string campaignId) const { return {UiCommandType::StartMarketingCampaign, 0, 0, 0, std::move(campaignId)}; }
UiCommand EconomyDashboard::acceptContractCommand(std::string contractId) const { return {UiCommandType::AcceptContract, 0, 0, 0, std::move(contractId)}; }
void EconomyDashboard::applyCommandResult(const UiCommandResult& result) { if (result.ok) { lastRejectionCode_.clear(); lastMessage_ = result.message; return; } lastRejectionCode_ = result.reasonCode; lastMessage_ = result.message; }
} // namespace hh::frontend
