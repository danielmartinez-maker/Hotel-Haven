#include "hh/game/RevenueManagement.h"

#include <algorithm>
#include <iomanip>
#include <sstream>
#include <stdexcept>

namespace hh::game {

PricingRuleResult RevenueManagement::setRule(const PricingRuleCommand &rule) {
  if (rule.ruleId == 0 || rule.startDay < 0 || rule.endDay < rule.startDay ||
      rule.roomCategory.empty() || rule.minOccupancyBasisPoints < 0 ||
      rule.maxOccupancyBasisPoints > 10000 ||
      rule.minOccupancyBasisPoints > rule.maxOccupancyBasisPoints ||
      rule.rateCents <= 0 || rule.minRateCents <= 0 ||
      rule.maxRateCents < rule.minRateCents || rule.weekdayMask == 0 ||
      (rule.weekdayMask & ~0x7fu) != 0)
    return {false, "INVALID_PRICING_RULE"};
  if (std::any_of(rules_.begin(), rules_.end(), [&](const auto &existing) {
        return existing.ruleId == rule.ruleId;
      }))
    return {false, "DUPLICATE_RULE_ID"};
  rules_.push_back(rule);
  std::sort(rules_.begin(), rules_.end(), [](const auto &a, const auto &b) {
    if (a.priority != b.priority) return a.priority > b.priority;
    return a.ruleId < b.ruleId;
  });
  return {true, "OK"};
}

PricingRuleResult RevenueManagement::removeRule(std::uint64_t ruleId) {
  const auto before = rules_.size();
  rules_.erase(std::remove_if(rules_.begin(), rules_.end(), [&](const auto &rule) {
                 return rule.ruleId == ruleId;
               }),
               rules_.end());
  return before == rules_.size() ? PricingRuleResult{false, "RULE_NOT_FOUND"}
                                 : PricingRuleResult{true, "OK"};
}

std::int64_t RevenueManagement::effectiveRateCents(int day, int weekday,
                                                    std::string_view roomCategory,
                                                    int projectedOccupancyBasisPoints,
                                                    std::int64_t baseRateCents) {
  const int normalizedWeekday = ((weekday % 7) + 7) % 7;
  std::int64_t result = baseRateCents;
  for (const auto &rule : rules_) {
    if (day < rule.startDay || day > rule.endDay || rule.roomCategory != roomCategory)
      continue;
    if ((rule.weekdayMask & (1u << normalizedWeekday)) == 0)
      continue;
    if (projectedOccupancyBasisPoints < rule.minOccupancyBasisPoints ||
        projectedOccupancyBasisPoints > rule.maxOccupancyBasisPoints)
      continue;
    result = std::clamp(rule.rateCents, rule.minRateCents, rule.maxRateCents);
    break;
  }
  lastEffectiveRate_[std::string(roomCategory)] = result;
  return result;
}

void RevenueManagement::setComparableMedianCents(std::string category,
                                                  std::int64_t cents) {
  if (category.empty() || cents <= 0)
    throw std::invalid_argument("invalid comparable median rate");
  comparableMedian_[std::move(category)] = cents;
}

RevenueManagementSnapshot RevenueManagement::snapshot() const {
  RevenueManagementSnapshot out;
  out.rules = rules_;
  out.comparableMedianRateCents = comparableMedian_;
  out.effectiveRateCents = lastEffectiveRate_;
  for (const auto &[category, median] : comparableMedian_) {
    const auto it = lastEffectiveRate_.find(category);
    if (it == lastEffectiveRate_.end() || median <= 0) {
      out.pricePositions[category] = PricePosition::Unknown;
      continue;
    }
    const auto rate = it->second;
    if (rate * 100 < median * 95)
      out.pricePositions[category] = PricePosition::BelowMarket;
    else if (rate * 100 > median * 105)
      out.pricePositions[category] = PricePosition::AboveMarket;
    else
      out.pricePositions[category] = PricePosition::AtMarket;
  }
  return out;
}

std::string RevenueManagement::save() const {
  std::ostringstream out;
  out << "HHRM 1 " << rules_.size();
  for (const auto &r : rules_)
    out << ' ' << r.ruleId << ' ' << r.startDay << ' ' << r.endDay << ' '
        << r.weekdayMask << ' ' << std::quoted(r.roomCategory) << ' '
        << r.minOccupancyBasisPoints << ' ' << r.maxOccupancyBasisPoints << ' '
        << r.rateCents << ' ' << r.priority << ' ' << r.minRateCents << ' ' << r.maxRateCents;
  out << ' ' << comparableMedian_.size();
  for (const auto &[category, cents] : comparableMedian_)
    out << ' ' << std::quoted(category) << ' ' << cents;
  out << ' ' << lastEffectiveRate_.size();
  for (const auto &[category, cents] : lastEffectiveRate_)
    out << ' ' << std::quoted(category) << ' ' << cents;
  return out.str();
}

RevenueManagement RevenueManagement::load(std::string_view data) {
  std::istringstream in{std::string(data)};
  std::string magic;
  int version{};
  std::size_t count{};
  in >> magic >> version >> count;
  if (!in || magic != "HHRM" || version != 1 || count > 100000)
    throw std::invalid_argument("invalid revenue management save");
  RevenueManagement result;
  for (std::size_t i = 0; i < count; ++i) {
    PricingRuleCommand r;
    in >> r.ruleId >> r.startDay >> r.endDay >> r.weekdayMask >> std::quoted(r.roomCategory)
       >> r.minOccupancyBasisPoints >> r.maxOccupancyBasisPoints >> r.rateCents
       >> r.priority >> r.minRateCents >> r.maxRateCents;
    if (!in || !result.setRule(r).ok)
      throw std::invalid_argument("invalid saved pricing rule");
  }
  in >> count;
  if (!in || count > 10000) throw std::invalid_argument("invalid comparable rate count");
  for (std::size_t i = 0; i < count; ++i) {
    std::string category;
    std::int64_t cents{};
    in >> std::quoted(category) >> cents;
    result.setComparableMedianCents(category, cents);
  }
  in >> count;
  if (!in || count > 10000) throw std::invalid_argument("invalid effective rate count");
  for (std::size_t i = 0; i < count; ++i) {
    std::string category;
    std::int64_t cents{};
    in >> std::quoted(category) >> cents;
    if (!in || category.empty() || cents <= 0)
      throw std::invalid_argument("invalid saved effective rate");
    result.lastEffectiveRate_[std::move(category)] = cents;
  }
  in >> std::ws;
  if (!in.eof()) throw std::invalid_argument("unexpected revenue management trailing data");
  return result;
}

} // namespace hh::game
