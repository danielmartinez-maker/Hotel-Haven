#include "hh/game/Financing.h"
#include <stdexcept>

using namespace hh::game;
static void require(bool v, const char *m) { if (!v) throw std::runtime_error(m); }

int main() {
  FinancingSystem financing;
  LoanOffer offer;
  offer.id = 7;
  offer.principalCents = 1200000;
  offer.annualInterestBasisPoints = 1200;
  offer.termMonths = 12;
  offer.paymentFrequencyDays = 30;
  offer.originationFeeCents = 12000;
  offer.minimumCashCents = 100000;
  const auto accepted = financing.acceptLoan(offer, 500000);
  require(accepted.ok && accepted.netProceedsCents == 1188000,
          "loan proceeds/origination fee incorrect");
  require(financing.snapshot().outstandingPrincipalCents == 1200000,
          "loan principal not recorded");

  const auto due = financing.processDay(30, 500000);
  require(due.debtServiceCents > 0 && due.principalPaidCents > 0 && due.interestPaidCents > 0,
          "scheduled amortization did not split principal and interest");
  require(financing.snapshot().outstandingPrincipalCents < 1200000,
          "principal did not amortize");

  FinancingSystem distress;
  distress.setCurePeriodDays(2);
  distress.observeDay(1, 50000, 100000, false);
  require(distress.snapshot().distressStage == DistressStage::Tight,
          "cash runway did not enter Tight stage");
  distress.observeDay(2, -1, 100000, true);
  require(distress.snapshot().distressStage == DistressStage::Default,
          "missed obligation did not remain inspectable as Default");
  distress.observeDay(4, -1, 100000, true);
  require(distress.snapshot().distressStage == DistressStage::Receivership,
          "uncured default did not advance to receivership");
}
