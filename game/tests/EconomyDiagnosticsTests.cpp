#include "hh/game/EconomyRuntime.h"
#include <cmath>
#include <stdexcept>

using namespace hh::game;
static void require(bool v, const char *m) { if (!v) throw std::runtime_error(m); }

int main() {
  EconomyRuntime runtime(808, 5'000'000);
  runtime.setPhysicalRoomCapacity("standard", 40);
  runtime.setPlayerHotelOffer({1, 15'000, 78, 4, 82, 80, 76, true});
  runtime.setCompetitors({{2, "Value", 16'000, 72, 4, 75, 76, 70},
                          {3, "Premium", 18'000, 84, 4, 88, 86, 82}});
  LoanOffer loan;
  loan.id = 90;
  loan.principalCents = 1'000'000;
  loan.annualInterestBasisPoints = 1200;
  loan.termMonths = 12;
  loan.paymentFrequencyDays = 30;
  require(runtime.acceptLoan(loan).ok, "diagnostic fixture loan rejected");
  runtime.runDays(10);

  const auto diagnostics = runtime.diagnostics();
  const auto financial = runtime.financialSnapshot();
  require(diagnostics.roomRevenueCents == financial.economics.roomRevenueCents &&
              diagnostics.totalRevenueCents == financial.economics.totalRevenueCents,
          "diagnostic revenue diverged from ledger snapshot");
  require(diagnostics.adrCents == financial.economics.adrCents &&
              diagnostics.revParCents == financial.economics.revParCents,
          "diagnostic ADR/RevPAR diverged from ledger snapshot");
  require(diagnostics.occupancyTodayBasisPoints >= 0 &&
              diagnostics.occupancyTodayBasisPoints <= 10000 &&
              diagnostics.occupancy7DayBasisPoints >= 0 &&
              diagnostics.occupancy7DayBasisPoints <= 10000 &&
              diagnostics.occupancy30DayBasisPoints >= 0 &&
              diagnostics.occupancy30DayBasisPoints <= 10000,
          "diagnostic occupancy window left basis-point bounds");
  require(diagnostics.competitorMedianRateCents == 17'000,
          "diagnostic comparable-set median changed");
  require(!diagnostics.bookingPaceByArrivalDay.empty(),
          "diagnostic booking pace is empty despite generated bookings");
  require(!diagnostics.demandBySegment.empty(),
          "diagnostic segment demand is empty despite generated requests");
  require(!diagnostics.channelBookings.empty(),
          "diagnostic channel mix is empty despite generated bookings");
  require(diagnostics.channelCommissionCents >= 0,
          "diagnostic commission cost became negative");
  require(std::isfinite(diagnostics.cashRunwayDays) &&
              diagnostics.cashRunwayDays > 0.0,
          "diagnostic cash runway is not finite/positive for funded fixture");
  require(diagnostics.outstandingPrincipalCents ==
              financial.financing.outstandingPrincipalCents &&
              diagnostics.nextDebtServiceCents ==
                  financial.financing.nextDebtServiceCents &&
              diagnostics.nextDebtPaymentDay == financial.financing.nextPaymentDay,
          "diagnostic debt schedule diverged from financing snapshot");
}
