#include "hh/game/Financing.h"
#include "hh/game/MarketDemand.h"
#include "hh/game/Overbooking.h"
#include <cmath>
#include <stdexcept>

using namespace hh::game;
static void require(bool v, const char *m) { if (!v) throw std::runtime_error(m); }
static void requireNear(double a, double b, double eps, const char *m) {
  if (std::abs(a - b) > eps) throw std::runtime_error(m);
}

int main() {
  const MarketSegment segments[] = {
      MarketSegment::BudgetLeisure,
      MarketSegment::Business,
      MarketSegment::ExecutiveBusiness,
      MarketSegment::CoupleLeisure,
      MarketSegment::FamilyLeisure,
      MarketSegment::LuxuryLeisure,
      MarketSegment::ConferenceGroup,
      MarketSegment::AirportTransit,
      MarketSegment::Wellness,
  };
  require(sizeof(segments) / sizeof(segments[0]) == 9,
          "HMG-030 requires nine baseline market segments");

  MarketDemandSystem market(3006);
  BookingRequest request;
  request.id = 1;
  request.segment = MarketSegment::BudgetLeisure;
  request.arrivalDay = 10;
  request.departureDay = 12;
  request.budgetCents = 20'000;
  MarketHotelOffer hotel{1, 15'000, 75, 4, 75, 75, 75, true};
  requireNear(market.priceUtility(request, hotel), 1.0, 1e-9,
              "price utility at 0.75 budget ratio must be 1.00");
  hotel.nightlyRateCents = 20'000;
  requireNear(market.priceUtility(request, hotel), 0.80, 1e-9,
              "price utility at 1.00 budget ratio must be 0.80");
  hotel.nightlyRateCents = 25'000;
  requireNear(market.priceUtility(request, hotel), 0.25, 1e-9,
              "price utility at 1.25 budget ratio must be 0.25");
  hotel.nightlyRateCents = 30'000;
  requireNear(market.priceUtility(request, hotel), 0.0, 1e-9,
              "price utility at 1.50 budget ratio must be 0.00");
  hotel.nightlyRateCents = 30'001;
  require(!market.isEligible(request, hotel),
          "hotel above 1.50x request budget must be ineligible");

  FinancingSystem financing;
  LoanOffer offer;
  offer.id = 44;
  offer.principalCents = 1'000'000;
  offer.annualInterestBasisPoints = 1200;
  offer.termMonths = 12;
  offer.paymentFrequencyDays = 30;
  const auto quote = financing.quoteLoan(offer);
  require(quote.ok, "valid amortizing loan should produce a quote");
  require(quote.nextPaymentCents == 88'849,
          "12-month 12% amortizing loan payment changed");
  require(quote.totalInterestCents == 66'188,
          "quoted total interest must reconcile to rounded payment schedule");
  require(financing.acceptLoan(offer, 0).ok, "valid quoted loan was rejected");
  std::int64_t principalPaid{};
  std::int64_t interestPaid{};
  for (int day = 30; day <= 360; day += 30) {
    const auto paid = financing.processDay(day, 10'000'000);
    require(paid.missedObligationCents == 0,
            "funded amortizing loan unexpectedly missed payment");
    principalPaid += paid.principalPaidCents;
    interestPaid += paid.interestPaidCents;
  }
  require(principalPaid == offer.principalCents,
          "amortizing schedule did not retire exact principal");
  require(interestPaid == quote.totalInterestCents,
          "realized interest diverged from pre-acceptance quote");
  require(financing.snapshot().outstandingPrincipalCents == 0,
          "loan principal remained after final scheduled payment");

  FinancingSystem tight;
  tight.observeDay(0, 29'999, 1'000, false);
  require(tight.snapshot().distressStage == DistressStage::Tight,
          "cash below 30 operating days must be Tight");
  FinancingSystem critical;
  critical.observeDay(0, 6'999, 1'000, false);
  require(critical.snapshot().distressStage == DistressStage::Critical,
          "cash below 7 operating days must be Critical");
  FinancingSystem insolvent;
  insolvent.observeDay(0, -1, 1'000, false);
  require(insolvent.snapshot().distressStage == DistressStage::Insolvent,
          "negative cash must be Insolvent before missed-obligation default");

  OverbookingSystem overbooking;
  require(overbooking.setPolicy({"standard", 2, 30'000, 10, 20}).ok,
          "date-scoped overbooking policy rejected");
  require(overbooking.allowance("standard", 9) == 0,
          "overbooking allowance leaked before policy window");
  require(overbooking.allowance("standard", 10) == 2 &&
              overbooking.allowance("standard", 20) == 2,
          "overbooking allowance missing inside policy window");
  require(overbooking.allowance("standard", 21) == 0,
          "overbooking allowance leaked after policy window");
}
