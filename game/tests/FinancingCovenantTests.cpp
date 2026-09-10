#include "hh/game/Financing.h"
#include <stdexcept>

using namespace hh::game;
static void require(bool v, const char *m) { if (!v) throw std::runtime_error(m); }

int main() {
  FinancingSystem financing;
  LoanOffer offer;
  offer.id = 700;
  offer.principalCents = 1'000'000;
  offer.annualInterestBasisPoints = 600;
  offer.termMonths = 24;
  offer.paymentFrequencyDays = 30;
  offer.covenants.push_back({"Max 4x debt/GOP", 50'000, 40'000});
  require(financing.acceptLoan(offer, 500'000).ok,
          "valid covenant loan was rejected");

  financing.observeDay(1, 500'000, 10'000, false, 300'000);
  require(!financing.snapshot().covenantBreach,
          "debt/GOP covenant breached below configured maximum");
  financing.observeDay(2, 500'000, 10'000, false, 200'000);
  require(financing.snapshot().covenantBreach &&
              financing.snapshot().distressStage == DistressStage::Tight,
          "debt/GOP covenant breach was not surfaced in distress state");
  financing.observeDay(3, 500'000, 10'000, false, 400'000);
  require(!financing.snapshot().covenantBreach,
          "debt/GOP covenant did not cure after GOP recovery");

  FinancingSystem nonPositiveGop;
  offer.id = 701;
  require(nonPositiveGop.acceptLoan(offer, 500'000).ok,
          "second covenant loan was rejected");
  nonPositiveGop.observeDay(1, 500'000, 10'000, false, 0);
  require(nonPositiveGop.snapshot().covenantBreach,
          "positive debt with non-positive GOP did not breach max debt/GOP covenant");

  LoanOffer invalid = offer;
  invalid.id = 702;
  invalid.covenants = {{"Invalid", -1, -1}};
  require(!financing.acceptLoan(invalid, 500'000).ok,
          "invalid negative covenant limits were accepted");
}
