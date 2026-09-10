#pragma once

#include <cstdint>
#include <map>
#include <string>
#include <string_view>
#include <vector>

namespace hh::game {

enum class PricePosition : std::uint8_t { Unknown, BelowMarket, AtMarket, AboveMarket };

struct PricingRuleCommand {
  std::uint64_t ruleId{};
  int startDay{};
  int endDay{};
  unsigned weekdayMask{0x7f};
  std::string roomCategory;
  int minOccupancyBasisPoints{};
  int maxOccupancyBasisPoints{10000};
  std::int64_t rateCents{};
  int priority{};
  std::int64_t minRateCents{};
  std::int64_t maxRateCents{};
  bool operator==(const PricingRuleCommand &) const = default;
};

struct PricingRuleResult {
  bool ok{};
  std::string reason;
  explicit operator bool() const noexcept { return ok; }
};

struct RevenueManagementSnapshot {
  std::vector<PricingRuleCommand> rules;
  std::map<std::string, std::int64_t> comparableMedianRateCents;
  std::map<std::string, PricePosition> pricePositions;
  std::map<std::string, std::int64_t> effectiveRateCents;
};

class RevenueManagement {
public:
  [[nodiscard]] PricingRuleResult setRule(const PricingRuleCommand &rule);
  [[nodiscard]] PricingRuleResult removeRule(std::uint64_t ruleId);
  [[nodiscard]] std::int64_t effectiveRateCents(int day, int weekday,
                                                std::string_view roomCategory,
                                                int projectedOccupancyBasisPoints,
                                                std::int64_t baseRateCents);
  void setComparableMedianCents(std::string category, std::int64_t cents);
  [[nodiscard]] RevenueManagementSnapshot snapshot() const;
  [[nodiscard]] std::string save() const;
  static RevenueManagement load(std::string_view data);

private:
  std::vector<PricingRuleCommand> rules_;
  std::map<std::string, std::int64_t> comparableMedian_;
  std::map<std::string, std::int64_t> lastEffectiveRate_;
};

} // namespace hh::game
