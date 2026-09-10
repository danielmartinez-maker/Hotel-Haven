#include "hh/game/Financing.h"

#include <algorithm>
#include <iomanip>
#include <sstream>
#include <stdexcept>

namespace hh::game {

std::int64_t FinancingSystem::interestDue(const ActiveLoan &loan) {
  const auto days = std::max(1, loan.offer.paymentFrequencyDays);
  const auto numerator = loan.remainingPrincipalCents *
                         static_cast<std::int64_t>(loan.offer.annualInterestBasisPoints) * days;
  return (numerator + 3650000 / 2) / 3650000;
}

std::int64_t FinancingSystem::principalDue(const ActiveLoan &loan) {
  if (loan.paymentsRemaining <= 1)
    return loan.remainingPrincipalCents;
  return (loan.remainingPrincipalCents + loan.paymentsRemaining - 1) /
         loan.paymentsRemaining;
}

LoanResult FinancingSystem::acceptLoan(const LoanOffer &offer,
                                       std::int64_t currentCashCents) {
  if (offer.id == 0 || offer.principalCents <= 0 ||
      offer.annualInterestBasisPoints < 0 || offer.annualInterestBasisPoints > 100000 ||
      offer.termMonths <= 0 || offer.paymentFrequencyDays <= 0 ||
      offer.originationFeeCents < 0 || offer.originationFeeCents >= offer.principalCents ||
      offer.minimumCashCents < 0)
    return {false, "INVALID_LOAN_OFFER", 0, 0};
  if (std::any_of(loans_.begin(), loans_.end(), [&](const auto &loan) {
        return loan.offer.id == offer.id;
      }))
    return {false, "LOAN_ALREADY_ACCEPTED", 0, 0};
  if (currentCashCents < offer.minimumCashCents)
    return {false, "MINIMUM_CASH_COVENANT_NOT_MET", 0, 0};

  ActiveLoan loan;
  loan.offer = offer;
  loan.remainingPrincipalCents = offer.principalCents;
  const int termDays = std::max(1, offer.termMonths * 30);
  loan.paymentsRemaining = std::max(1, (termDays + offer.paymentFrequencyDays - 1) /
                                         offer.paymentFrequencyDays);
  loan.nextPaymentDay = offer.paymentFrequencyDays;
  loans_.push_back(std::move(loan));
  std::sort(loans_.begin(), loans_.end(), [](const auto &a, const auto &b) {
    return a.offer.id < b.offer.id;
  });
  return {true, "OK", offer.id, offer.principalCents - offer.originationFeeCents};
}

DebtServiceResult FinancingSystem::processDay(int day,
                                              std::int64_t currentCashCents) {
  DebtServiceResult result;
  for (auto &loan : loans_) {
    if (loan.remainingPrincipalCents <= 0 || day < loan.nextPaymentDay)
      continue;
    while (loan.remainingPrincipalCents > 0 && day >= loan.nextPaymentDay) {
      const auto interest = interestDue(loan);
      const auto principal = std::min(loan.remainingPrincipalCents, principalDue(loan));
      const auto due = interest + principal;
      if (currentCashCents - result.debtServiceCents < due) {
        result.missedObligationCents += due;
        missedObligationCents_ += due;
        if (defaultStartDay_ < 0)
          defaultStartDay_ = day;
        distressStage_ = DistressStage::Default;
        break;
      }
      result.debtServiceCents += due;
      result.principalPaidCents += principal;
      result.interestPaidCents += interest;
      loan.remainingPrincipalCents -= principal;
      --loan.paymentsRemaining;
      loan.nextPaymentDay += loan.offer.paymentFrequencyDays;
    }
  }
  return result;
}

void FinancingSystem::setCurePeriodDays(int days) {
  if (days <= 0 || days > 3650)
    throw std::invalid_argument("invalid cure period");
  curePeriodDays_ = days;
}

void FinancingSystem::observeDay(int day, std::int64_t cashCents,
                                 std::int64_t averageDailyOperatingCostCents,
                                 bool missedObligation) {
  if (missedObligation) {
    if (defaultStartDay_ < 0)
      defaultStartDay_ = day;
    distressStage_ = day - defaultStartDay_ >= curePeriodDays_
                         ? DistressStage::Receivership
                         : DistressStage::Default;
    return;
  }
  if (distressStage_ == DistressStage::Default ||
      distressStage_ == DistressStage::Receivership) {
    if (distressStage_ == DistressStage::Default && defaultStartDay_ >= 0 &&
        day - defaultStartDay_ >= curePeriodDays_)
      distressStage_ = DistressStage::Receivership;
    return;
  }

  covenantBreach_ = false;
  for (const auto &loan : loans_) {
    const auto minimum = std::max(loan.offer.minimumCashCents,
                                  loan.offer.covenants.empty()
                                      ? std::int64_t{0}
                                      : loan.offer.covenants.front().minimumCashCents);
    if (cashCents < minimum)
      covenantBreach_ = true;
  }

  if (cashCents < 0) {
    distressStage_ = DistressStage::Insolvent;
    return;
  }
  if (averageDailyOperatingCostCents <= 0) {
    distressStage_ = covenantBreach_ ? DistressStage::Tight : DistressStage::Healthy;
    return;
  }
  const double runwayDays = static_cast<double>(cashCents) /
                            static_cast<double>(averageDailyOperatingCostCents);
  if (runwayDays < 0.5)
    distressStage_ = DistressStage::Critical;
  else if (runwayDays < 14.0 || covenantBreach_)
    distressStage_ = DistressStage::Tight;
  else
    distressStage_ = DistressStage::Healthy;
}

FinancingSnapshot FinancingSystem::snapshot() const {
  FinancingSnapshot out;
  out.distressStage = distressStage_;
  out.defaultStartDay = defaultStartDay_;
  out.missedObligationCents = missedObligationCents_;
  out.covenantBreach = covenantBreach_;
  int nextPayment = 0;
  for (const auto &loan : loans_) {
    if (loan.remainingPrincipalCents <= 0)
      continue;
    out.outstandingPrincipalCents += loan.remainingPrincipalCents;
    const auto due = interestDue(loan) + principalDue(loan);
    out.nextDebtServiceCents += due;
    if (nextPayment == 0 || loan.nextPaymentDay < nextPayment)
      nextPayment = loan.nextPaymentDay;
  }
  out.nextPaymentDay = nextPayment;
  return out;
}

std::string FinancingSystem::save() const {
  std::ostringstream out;
  out << "HHFIN 1 " << static_cast<int>(distressStage_) << ' ' << defaultStartDay_ << ' '
      << curePeriodDays_ << ' ' << missedObligationCents_ << ' ' << covenantBreach_ << ' '
      << loans_.size();
  for (const auto &loan : loans_) {
    const auto &o = loan.offer;
    out << ' ' << o.id << ' ' << o.principalCents << ' ' << o.annualInterestBasisPoints << ' '
        << o.termMonths << ' ' << o.paymentFrequencyDays << ' ' << o.originationFeeCents << ' '
        << std::quoted(o.collateralRule) << ' ' << o.minimumCashCents << ' '
        << loan.remainingPrincipalCents << ' ' << loan.paymentsRemaining << ' '
        << loan.nextPaymentDay << ' ' << o.covenants.size();
    for (const auto &covenant : o.covenants)
      out << ' ' << std::quoted(covenant.name) << ' ' << covenant.minimumCashCents << ' '
          << covenant.maximumDebtToGopBasisPoints;
  }
  return out.str();
}

FinancingSystem FinancingSystem::load(std::string_view data) {
  std::istringstream in{std::string(data)};
  std::string magic;
  int version{}, stage{};
  FinancingSystem result;
  std::size_t count{};
  in >> magic >> version >> stage >> result.defaultStartDay_ >> result.curePeriodDays_ >>
      result.missedObligationCents_ >> result.covenantBreach_ >> count;
  if (!in || magic != "HHFIN" || version != 1 || stage < 0 ||
      stage > static_cast<int>(DistressStage::Receivership) || count > 10000 ||
      result.curePeriodDays_ <= 0)
    throw std::invalid_argument("invalid financing save");
  result.distressStage_ = static_cast<DistressStage>(stage);
  for (std::size_t i = 0; i < count; ++i) {
    ActiveLoan loan;
    std::size_t covenantCount{};
    in >> loan.offer.id >> loan.offer.principalCents >> loan.offer.annualInterestBasisPoints >>
        loan.offer.termMonths >> loan.offer.paymentFrequencyDays >> loan.offer.originationFeeCents >>
        std::quoted(loan.offer.collateralRule) >> loan.offer.minimumCashCents >>
        loan.remainingPrincipalCents >> loan.paymentsRemaining >> loan.nextPaymentDay >> covenantCount;
    if (!in || loan.offer.id == 0 || loan.offer.principalCents <= 0 ||
        loan.remainingPrincipalCents < 0 || loan.paymentsRemaining < 0 || covenantCount > 1000)
      throw std::invalid_argument("invalid saved loan");
    for (std::size_t j = 0; j < covenantCount; ++j) {
      LoanCovenant covenant;
      in >> std::quoted(covenant.name) >> covenant.minimumCashCents >>
          covenant.maximumDebtToGopBasisPoints;
      if (!in) throw std::invalid_argument("invalid saved covenant");
      loan.offer.covenants.push_back(std::move(covenant));
    }
    result.loans_.push_back(std::move(loan));
  }
  in >> std::ws;
  if (!in.eof())
    throw std::invalid_argument("unexpected financing trailing data");
  return result;
}

} // namespace hh::game
