#pragma once
#include "hh/frontend/GameUiTypes.h"
#include <cstddef>
#include <cstdint>
#include <optional>
#include <string>
namespace hh::frontend {
class EconomyDashboard {
public:
    void update(const EconomySnapshot& snapshot) { snapshot_ = snapshot; }
    [[nodiscard]] const EconomySnapshot& snapshot() const noexcept { return snapshot_; }
    [[nodiscard]] static std::string formatMoney(std::int64_t cents);
    [[nodiscard]] UiCommand setFutureRateCommand(int day, std::string roomCategory, std::int64_t rateCents) const;
    [[nodiscard]] UiCommand setOverbookingCommand(int limit) const;
    [[nodiscard]] std::optional<UiCommand> adjustPricingRuleCommand(std::size_t index, std::int64_t deltaCents) const;
    [[nodiscard]] std::optional<UiCommand> adjustOverbookingCommand(std::size_t index, int delta) const;
    [[nodiscard]] UiCommand startCampaignCommand(std::string campaignId) const;
    [[nodiscard]] UiCommand acceptContractCommand(std::string contractId) const;
    void applyCommandResult(const UiCommandResult& result);
    [[nodiscard]] const std::string& lastRejectionCode() const noexcept { return lastRejectionCode_; }
    [[nodiscard]] const std::string& lastMessage() const noexcept { return lastMessage_; }
private: EconomySnapshot snapshot_{}; std::string lastRejectionCode_; std::string lastMessage_;
};
} // namespace hh::frontend
