#include "hh/game/Financing.h"

#include <algorithm>
#include <cmath>
#include <iomanip>
#include <limits>
#include <sstream>
#include <stdexcept>

namespace hh::game {
namespace {

bool validOffer(const LoanOffer &offer) {
  if (offer.id == 0 || offer.principalCents <= 0 ||
      offer.annualInterestBasisPoints < 0 ||
      offer.annualInterestBasisPoints > 100000 || offer.termMonths <= 0 ||
      offer.termMonths > 1200 || offer.paymentFrequencyDays <= 0 ||
      offer.paymentFrequencyDays > 3650 || offer.originationFeeCents < 0 ||
      offer.originationFeeCents >= offer.principalCents || offer.minimumCashCents < 0)
    return false;
  return std::all_of(offer.covenants.begin(), offer.covenants.end(),
                     [](const auto &covenant) {
                       return covenant.minimumCashCents >= 0 &&
                              covenant.maximumDebtToGopBasisPoints >= 0;
                     });
}

} // namespace

int FinancingSystem::paymentCount(const LoanOffer &offer) {
  const int termDays = offer.termMonths * 30;
  return std::max(1, (termDays + offer.paymentFrequencyDays - 1) /
                         offer.paymentFrequencyDays);
}

std::int64_t FinancingSystem::periodicInterestDue(const ActiveLoan &loan) {
  const long double rate =
      (static_cast<long double>(loan.offer.annualInterestBasisPoints) / 10000.0L) *
      (static_cast<long double>(loan.offer.paymentFrequencyDays) / 360.0L);
  return static_cast<std::int64_t>(
      std::llround(static_cast<long double>(loan.remainingPrincipalCents) * rate));
}

std::int64_t FinancingSystem::legacyInterestDue(const ActiveLoan &loan) {
  const auto days = std::max(1, loan.offer.paymentFrequencyDays);
  const long double amount =
      static_cast<long double>(loan.remainingPrincipalCents) *
      static_cast<long double>(loan.offer.annualInterestBasisPoints) *
      static_cast<long double>(days) / 3650000.0L;
  return static_cast<std::int64_t>(std::llround(amount));
}

std::int64_t FinancingSystem::legacyPrincipalDue(const ActiveLoan &loan) {
  if (loan.paymentsRemaining <= 1)
    return loan.remainingPrincipalCents;
  return (loan.remainingPrincipalCents + loan.paymentsRemaining - 1) /
         loan.paymentsRemaining;
}

std::int64_t FinancingSystem::nextPaymentDue(const ActiveLoan &loan) {
  if (loan.remainingPrincipalCents <= 0)
    return 0;
  if (loan.legacyEqualPrincipal)
    return legacyInterestDue(loan) + legacyPrincipalDue(loan);
  const auto remainingQuotedInterest =
      std::max<std::int64_t>(0, loan.quotedTotalInterestCents - loan.interestPaidCents);
  if (loan.paymentsRemaining <= 1)
    return loan.remainingPrincipalCents + remainingQuotedInterest;
  return loan.scheduledPaymentCents;
}

LoanQuote FinancingSystem::quoteLoan(const LoanOffer &offer) const {
  if (!validOffer(offer))
    return {false, "INVALID_LOAN_OFFER", 0, 0, 0, 0};
  const int count = paymentCount(offer);
  std::int64_t payment{};
  std::int64_t totalInterest{};
  std::int64_t totalRepayment{};
  if (offer.annualInterestBasisPoints == 0) {
    payment = (offer.principalCents + count - 1) / count;
    totalInterest = 0;
    totalRepayment = offer.principalCents;
  } else {
    const long double periodicRate =
        (static_cast<long double>(offer.annualInterestBasisPoints) / 10000.0L) *
        (static_cast<long double>(offer.paymentFrequencyDays) / 360.0L);
    const long double denominator =
        1.0L - std::pow(1.0L + periodicRate, -static_cast<long double>(count));
    if (!(denominator > 0.0L))
      return {false, "INVALID_AMORTIZATION", 0, 0, 0, 0};
    const long double rawPayment =
        static_cast<long double>(offer.principalCents) * periodicRate / denominator;
    if (rawPayment <= 0.0L ||
        rawPayment > static_cast<long double>(std::numeric_limits<std::int64_t>::max()))
      return {false, "AMORTIZATION_OVERFLOW", 0, 0, 0, 0};
    payment = static_cast<std::int64_t>(std::llround(rawPayment));
    const long double rawTotal = static_cast<long double>(payment) * count;
    if (rawTotal > static_cast<long double>(std::numeric_limits<std::int64_t>::max()))
      return {false, "AMORTIZATION_OVERFLOW", 0, 0, 0, 0};
    totalRepayment = payment * static_cast<std::int64_t>(count);
    totalInterest = std::max<std::int64_t>(0, totalRepayment - offer.principalCents);
  }
  return {true, "OK", count, payment, totalInterest, totalRepayment};
}

LoanResult FinancingSystem::acceptLoan(const LoanOffer &offer,
                                       std::int64_t currentCashCents) {
  const auto quote = quoteLoan(offer);
  if (!quote.ok)
    return {false, quote.reason, 0, 0};
  if (std::any_of(loans_.begin(), loans_.end(), [&](const auto &loan) {
        return loan.offer.id == offer.id;
      }))
    return {false, "LOAN_ALREADY_ACCEPTED", 0, 0};
  if (currentCashCents < offer.minimumCashCents ||
      std::any_of(offer.covenants.begin(), offer.covenants.end(),
                  [&](const auto &covenant) {
                    return currentCashCents < covenant.minimumCashCents;
                  }))
    return {false, "MINIMUM_CASH_COVENANT_NOT_MET", 0, 0};

  ActiveLoan loan;
  loan.offer = offer;
  loan.remainingPrincipalCents = offer.principalCents;
  loan.paymentsRemaining = quote.paymentCount;
  loan.nextPaymentDay = offer.paymentFrequencyDays;
  loan.scheduledPaymentCents = quote.nextPaymentCents;
  loan.quotedTotalInterestCents = quote.totalInterestCents;
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
      std::int64_t interest{};
      std::int64_t principal{};
      if (loan.legacyEqualPrincipal) {
        interest = legacyInterestDue(loan);
        principal = std::min(loan.remainingPrincipalCents, legacyPrincipalDue(loan));
      } else {
        const auto remainingQuotedInterest =
            std::max<std::int64_t>(0,
                loan.quotedTotalInterestCents - loan.interestPaidCents);
        if (loan.paymentsRemaining <= 1) {
          interest = remainingQuotedInterest;
          principal = loan.remainingPrincipalCents;
        } else {
          interest = std::min(periodicInterestDue(loan), remainingQuotedInterest);
          principal = std::min(
              loan.remainingPrincipalCents,
              std::max<std::int64_t>(1, loan.scheduledPaymentCents - interest));
        }
      }
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
      loan.interestPaidCents += interest;
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
                                 bool missedObligation,
                                 std::int64_t gopCents) {
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

  std::int64_t totalOutstandingPrincipal = 0;
  for (const auto &loan : loans_)
    totalOutstandingPrincipal += loan.remainingPrincipalCents;
  if (totalOutstandingPrincipal <= 0) {
    debtToGopBasisPoints_ = 0;
  } else if (gopCents <= 0) {
    debtToGopBasisPoints_ = std::numeric_limits<int>::max();
  } else {
    const long double rawRatio =
        static_cast<long double>(totalOutstandingPrincipal) * 10000.0L /
        static_cast<long double>(gopCents);
    debtToGopBasisPoints_ = rawRatio >= static_cast<long double>(std::numeric_limits<int>::max())
                                ? std::numeric_limits<int>::max()
                                : static_cast<int>(std::llround(rawRatio));
  }

  covenantBreach_ = false;
  for (const auto &loan : loans_) {
    if (cashCents < loan.offer.minimumCashCents)
      covenantBreach_ = true;
    for (const auto &covenant : loan.offer.covenants) {
      if (cashCents < covenant.minimumCashCents)
        covenantBreach_ = true;
      if (covenant.maximumDebtToGopBasisPoints > 0 &&
          totalOutstandingPrincipal > 0 &&
          debtToGopBasisPoints_ > covenant.maximumDebtToGopBasisPoints)
        covenantBreach_ = true;
    }
  }

  if (cashCents < 0) {
    distressStage_ = DistressStage::Insolvent;
    return;
  }
  if (averageDailyOperatingCostCents <= 0) {
    distressStage_ = covenantBreach_ ? DistressStage::Tight : DistressStage::Healthy;
    return;
  }
  const long double runwayDays = static_cast<long double>(cashCents) /
                                 static_cast<long double>(averageDailyOperatingCostCents);
  if (runwayDays < 7.0L)
    distressStage_ = DistressStage::Critical;
  else if (runwayDays < 30.0L || covenantBreach_)
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
  out.debtToGopBasisPoints = debtToGopBasisPoints_;
  int nextPayment = 0;
  for (const auto &loan : loans_) {
    if (loan.remainingPrincipalCents <= 0)
      continue;
    out.outstandingPrincipalCents += loan.remainingPrincipalCents;
    out.nextDebtServiceCents += nextPaymentDue(loan);
    if (nextPayment == 0 || loan.nextPaymentDay < nextPayment)
      nextPayment = loan.nextPaymentDay;
  }
  out.nextPaymentDay = nextPayment;
  return out;
}

std::string FinancingSystem::save() const {
  std::ostringstream out;
  out << "HHFIN 3 " << static_cast<int>(distressStage_) << ' ' << defaultStartDay_ << ' '
      << curePeriodDays_ << ' ' << missedObligationCents_ << ' ' << covenantBreach_ << ' '
      << debtToGopBasisPoints_ << ' ' << loans_.size();
  for (const auto &loan : loans_) {
    const auto &o = loan.offer;
    out << ' ' << o.id << ' ' << o.principalCents << ' ' << o.annualInterestBasisPoints << ' '
        << o.termMonths << ' ' << o.paymentFrequencyDays << ' ' << o.originationFeeCents << ' '
        << std::quoted(o.collateralRule) << ' ' << o.minimumCashCents << ' '
        << loan.remainingPrincipalCents << ' ' << loan.paymentsRemaining << ' '
        << loan.nextPaymentDay << ' ' << loan.scheduledPaymentCents << ' '
        << loan.quotedTotalInterestCents << ' ' << loan.interestPaidCents << ' '
        << loan.legacyEqualPrincipal << ' ' << o.covenants.size();
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
      result.missedObligationCents_ >> result.covenantBreach_;
  if (version >= 3)
    in >> result.debtToGopBasisPoints_;
  in >> count;
  if (!in || magic != "HHFIN" ||
      (version != 1 && version != 2 && version != 3) || stage < 0 ||
      stage > static_cast<int>(DistressStage::Receivership) || count > 10000 ||
      result.curePeriodDays_ <= 0 || result.debtToGopBasisPoints_ < 0)
    throw std::invalid_argument("invalid financing save");
  result.distressStage_ = static_cast<DistressStage>(stage);
  for (std::size_t i = 0; i < count; ++i) {
    ActiveLoan loan;
    std::size_t covenantCount{};
    in >> loan.offer.id >> loan.offer.principalCents >> loan.offer.annualInterestBasisPoints >>
        loan.offer.termMonths >> loan.offer.paymentFrequencyDays >> loan.offer.originationFeeCents >>
        std::quoted(loan.offer.collateralRule) >> loan.offer.minimumCashCents >>
        loan.remainingPrincipalCents >> loan.paymentsRemaining >> loan.nextPaymentDay;
    if (version >= 2) {
      in >> loan.scheduledPaymentCents >> loan.quotedTotalInterestCents >>
          loan.interestPaidCents >> loan.legacyEqualPrincipal;
    } else {
      loan.legacyEqualPrincipal = true;
    }
    in >> covenantCount;
    if (!in || loan.offer.id == 0 || loan.offer.principalCents <= 0 ||
        loan.remainingPrincipalCents < 0 || loan.paymentsRemaining < 0 ||
        loan.scheduledPaymentCents < 0 || loan.quotedTotalInterestCents < 0 ||
        loan.interestPaidCents < 0 || covenantCount > 1000)
      throw std::invalid_argument("invalid saved loan");
    for (std::size_t j = 0; j < covenantCount; ++j) {
      LoanCovenant covenant;
      in >> std::quoted(covenant.name) >> covenant.minimumCashCents >>
          covenant.maximumDebtToGopBasisPoints;
      if (!in || covenant.minimumCashCents < 0 ||
          covenant.maximumDebtToGopBasisPoints < 0)
        throw std::invalid_argument("invalid saved covenant");
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
