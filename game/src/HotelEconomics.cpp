#include "hh/game/HotelEconomics.h"

#include <algorithm>
#include <iomanip>
#include <set>
#include <sstream>
#include <stdexcept>

namespace hh::game {
namespace {

bool isRevenue(EconomicCategory category) {
  return category >= EconomicCategory::RoomRevenue &&
         category <= EconomicCategory::NoShowFeeRevenue;
}

bool isOperatingCost(EconomicCategory category) {
  return category >= EconomicCategory::LaborCost &&
         category <= EconomicCategory::CompensationCost;
}

bool validSign(EconomicCategory category, std::int64_t amount) {
  if (amount == 0)
    return false;
  if (isRevenue(category) || category == EconomicCategory::LoanProceeds)
    return amount > 0;
  return amount < 0;
}

} // namespace

HotelEconomics::HotelEconomics(std::int64_t openingCashCents)
    : openingCashCents_(openingCashCents) {}

void HotelEconomics::post(const EconomicTransaction &transaction) {
  if (transaction.id == 0 || transaction.day < 0 ||
      !validSign(transaction.category, transaction.amountCents))
    throw std::invalid_argument("invalid economic transaction");
  if (std::any_of(transactions_.begin(), transactions_.end(), [&](const auto &existing) {
        return existing.id == transaction.id;
      }))
    throw std::invalid_argument("duplicate economic transaction id");
  transactions_.push_back(transaction);
  std::sort(transactions_.begin(), transactions_.end(), [](const auto &a, const auto &b) {
    if (a.day != b.day) return a.day < b.day;
    return a.id < b.id;
  });
}

HotelEconomicsSnapshot HotelEconomics::snapshot(int throughDay,
                                                 std::int64_t sellableRoomNights,
                                                 std::int64_t occupiedRoomNights) const {
  HotelEconomicsSnapshot out;
  out.cashCents = openingCashCents_;
  for (const auto &transaction : transactions_) {
    if (transaction.day > throughDay)
      continue;
    out.cashCents += transaction.amountCents;
    ++out.transactionCount;
    if (isRevenue(transaction.category)) {
      out.totalRevenueCents += transaction.amountCents;
      if (transaction.category == EconomicCategory::RoomRevenue)
        out.roomRevenueCents += transaction.amountCents;
      else
        out.nonRoomRevenueCents += transaction.amountCents;
    } else if (isOperatingCost(transaction.category)) {
      out.operatingCostCents -= transaction.amountCents;
    } else if (transaction.category == EconomicCategory::DebtService) {
      out.debtServiceCents -= transaction.amountCents;
    } else if (transaction.category == EconomicCategory::CapitalExpense ||
               transaction.category == EconomicCategory::LoanOriginationFee) {
      out.capitalExpenseCents -= transaction.amountCents;
    }
  }
  out.gopCents = out.totalRevenueCents - out.operatingCostCents;
  if (sellableRoomNights > 0) {
    const auto boundedOccupied = std::clamp<std::int64_t>(occupiedRoomNights, 0, sellableRoomNights);
    out.occupancy = static_cast<double>(boundedOccupied) /
                    static_cast<double>(sellableRoomNights);
    out.revParCents = out.roomRevenueCents / sellableRoomNights;
    out.tRevParCents = out.totalRevenueCents / sellableRoomNights;
    if (boundedOccupied > 0)
      out.adrCents = out.roomRevenueCents / boundedOccupied;
  }
  out.reconciled = reconciles();
  return out;
}

bool HotelEconomics::reconciles() const {
  std::set<std::uint64_t> ids;
  for (const auto &transaction : transactions_) {
    if (transaction.id == 0 || transaction.day < 0 ||
        !validSign(transaction.category, transaction.amountCents) ||
        !ids.insert(transaction.id).second)
      return false;
  }
  return true;
}

const std::vector<EconomicTransaction> &HotelEconomics::transactions() const noexcept {
  return transactions_;
}

std::int64_t HotelEconomics::openingCashCents() const noexcept {
  return openingCashCents_;
}

std::string HotelEconomics::save() const {
  std::ostringstream out;
  out << "HHECON 1 " << openingCashCents_ << ' ' << transactions_.size();
  for (const auto &transaction : transactions_)
    out << ' ' << transaction.id << ' ' << transaction.day << ' '
        << static_cast<int>(transaction.category) << ' ' << transaction.amountCents << ' '
        << transaction.sourceId << ' ' << std::quoted(transaction.memo);
  return out.str();
}

HotelEconomics HotelEconomics::load(std::string_view data) {
  std::istringstream in{std::string(data)};
  std::string magic;
  int version{};
  std::int64_t openingCash{};
  std::size_t count{};
  in >> magic >> version >> openingCash >> count;
  if (!in || magic != "HHECON" || version != 1 || count > 2'000'000)
    throw std::invalid_argument("invalid hotel economics save");
  HotelEconomics result(openingCash);
  for (std::size_t i = 0; i < count; ++i) {
    EconomicTransaction transaction;
    int category{};
    in >> transaction.id >> transaction.day >> category >> transaction.amountCents >>
        transaction.sourceId >> std::quoted(transaction.memo);
    if (!in || category < static_cast<int>(EconomicCategory::RoomRevenue) ||
        category > static_cast<int>(EconomicCategory::LoanOriginationFee))
      throw std::invalid_argument("invalid saved economic transaction");
    transaction.category = static_cast<EconomicCategory>(category);
    result.post(transaction);
  }
  in >> std::ws;
  if (!in.eof())
    throw std::invalid_argument("unexpected hotel economics trailing data");
  return result;
}

} // namespace hh::game
