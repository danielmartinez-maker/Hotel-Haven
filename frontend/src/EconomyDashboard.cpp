#include "hh/frontend/EconomyDashboard.h"
#include <iomanip>
#include <sstream>
#include <utility>
namespace hh::frontend {
std::string EconomyDashboard::formatMoney(std::int64_t cents) { const bool negative = cents < 0; const std::uint64_t absolute = negative ? static_cast<std::uint64_t>(-(cents + 1)) + 1U : static_cast<std::uint64_t>(cents), dollars = absolute / 100, remainder = absolute % 100; std::string digits = std::to_string(dollars); for (std::ptrdiff_t i = static_cast<std::ptrdiff_t>(digits.size()) - 3; i > 0; i -= 3) digits.insert(static_cast<std::size_t>(i), ","); std::ostringstream out; if (negative) out << '-'; out << '$' << digits << '.' << std::setw(2) << std::setfill('0') << remainder; return out.str(); }
UiCommand EconomyDashboard::setFutureRateCommand(int day, std::string roomCategory, std::int64_t rateCents) const { return {UiCommandType::SetFutureRate, 0, rateCents, day, std::move(roomCategory)}; }
UiCommand EconomyDashboard::setOverbookingCommand(int limit) const { return {UiCommandType::SetOverbookingPolicy, 0, limit}; }
UiCommand EconomyDashboard::startCampaignCommand(std::string campaignId) const { return {UiCommandType::StartMarketingCampaign, 0, 0, 0, std::move(campaignId)}; }
UiCommand EconomyDashboard::acceptContractCommand(std::string contractId) const { return {UiCommandType::AcceptContract, 0, 0, 0, std::move(contractId)}; }
void EconomyDashboard::applyCommandResult(const UiCommandResult& result) { if (result.ok) { lastRejectionCode_.clear(); lastMessage_ = result.message; return; } lastRejectionCode_ = result.reasonCode; lastMessage_ = result.message; }
} // namespace hh::frontend
