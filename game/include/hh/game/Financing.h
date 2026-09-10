#pragma once

#include <cstdint>
#include <string>
#include <string_view>
#include <vector>

namespace hh::game {

using LoanOfferId = std::uint64_t;

enum class DistressStage : std::uint8_t {
  Healthy,
  Tight,
  Critical,
  Insolvent,
  Default,
  Receivership
};

struct LoanCovenant {
  std::string name;
  std::int64_t minimumCashCents{};
  int maximumDebtToGopBasisPoints{};
};

struct LoanOffer {
  LoanOfferId id{};
  std::int64_t principalCents{};
  int annualInterestBasisPoints{};
  int termMonths{};
  int paymentFrequencyDays{30};
  std::int64_t originationFeeCents{};
  std::string collateralRule;
  std::vector<LoanCovenant> covenants;
  std::int64_t minimumCashCents{};
};

struct LoanResult {
  bool ok{};
  std::string reason;
  LoanOfferId loanId{};
  std::int64_t netProceedsCents{};
  explicit operator bool() const noexcept { return ok; }
};

struct DebtServiceResult {
  std::int64_t debtServiceCents{};
  std::int64_t principalPaidCents{};
  std::int64_t interestPaidCents{};
  std::int64_t missedObligationCents{};
};

struct FinancingSnapshot {
  std::int64_t outstandingPrincipalCents{};
  std::int64_t nextDebtServiceCents{};
  std::int64_t missedObligationCents{};
  int nextPaymentDay{};
  DistressStage distressStage{DistressStage::Healthy};
  int defaultStartDay{-1};
  bool covenantBreach{};
};

class FinancingSystem {
public:
  [[nodiscard]] LoanResult acceptLoan(const LoanOffer &offer,
                                      std::int64_t currentCashCents);
  [[nodiscard]] DebtServiceResult processDay(int day,
                                             std::int64_t currentCashCents);
  void setCurePeriodDays(int days);
  void observeDay(int day, std::int64_t cashCents,
                  std::int64_t averageDailyOperatingCostCents,
                  bool missedObligation);
  [[nodiscard]] FinancingSnapshot snapshot() const;
  [[nodiscard]] std::string save() const;
  static FinancingSystem load(std::string_view data);

private:
  struct ActiveLoan {
    LoanOffer offer;
    std::int64_t remainingPrincipalCents{};
    int paymentsRemaining{};
    int nextPaymentDay{};
  };

  std::vector<ActiveLoan> loans_;
  DistressStage distressStage_{DistressStage::Healthy};
  int defaultStartDay_{-1};
  int curePeriodDays_{14};
  std::int64_t missedObligationCents_{};
  bool covenantBreach_{};

  [[nodiscard]] static std::int64_t interestDue(const ActiveLoan &loan);
  [[nodiscard]] static std::int64_t principalDue(const ActiveLoan &loan);
};

} // namespace hh::game
