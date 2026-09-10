#include "hh/game/EconomyRuntime.h"
#include <stdexcept>

using namespace hh::game;
static void require(bool value, const char *message) {
  if (!value) throw std::runtime_error(message);
}

int main() {
  EconomyRuntime runtime(606, 2'000'000);
  runtime.setPhysicalRoomCapacity("standard", 20);

  runtime.postExternalTransaction(0, EconomicCategory::RoomRevenue,
                                  EconomicDepartment::Rooms, 100'000, 1,
                                  "rooms diagnostic revenue");
  runtime.postExternalTransaction(0, EconomicCategory::LaborCost,
                                  EconomicDepartment::Rooms, -20'000, 2,
                                  "rooms diagnostic labor");
  runtime.postExternalTransaction(0, EconomicCategory::RestaurantFoodRevenue,
                                  EconomicDepartment::FoodBeverage, 50'000, 3,
                                  "fnb diagnostic revenue");
  runtime.postExternalTransaction(0, EconomicCategory::FoodBeverageCost,
                                  EconomicDepartment::FoodBeverage, -15'000, 4,
                                  "fnb diagnostic cost");

  LoanOffer loan;
  loan.id = 77;
  loan.principalCents = 1'200'000;
  loan.annualInterestBasisPoints = 1200;
  loan.termMonths = 12;
  loan.paymentFrequencyDays = 30;
  require(runtime.acceptLoan(loan).ok, "diagnostic loan rejected");

  const auto diagnostics = runtime.diagnostics();
  const auto rooms = diagnostics.departmentContribution.find(EconomicDepartment::Rooms);
  const auto fnb = diagnostics.departmentContribution.find(EconomicDepartment::FoodBeverage);
  require(rooms != diagnostics.departmentContribution.end() &&
              rooms->second.revenueCents == 100'000 &&
              rooms->second.expenseCents == 20'000 &&
              rooms->second.contributionCents == 80'000,
          "rooms department contribution did not reconcile from ledger rows");
  require(fnb != diagnostics.departmentContribution.end() &&
              fnb->second.revenueCents == 50'000 &&
              fnb->second.expenseCents == 15'000 &&
              fnb->second.contributionCents == 35'000,
          "F&B department contribution did not reconcile from ledger rows");

  require(!diagnostics.debtPaymentSchedule.empty(),
          "full debt payment schedule missing from diagnostics");
  require(diagnostics.debtPaymentSchedule.front().loanId == loan.id &&
              diagnostics.debtPaymentSchedule.front().paymentDay == 30 &&
              diagnostics.debtPaymentSchedule.front().amountCents ==
                  runtime.financialSnapshot().financing.nextDebtServiceCents,
          "first diagnostic debt payment diverged from financing snapshot");
  require(diagnostics.debtPaymentSchedule.size() == 12,
          "amortizing 12-month loan did not expose all future payments");

  const auto saved = runtime.save();
  const auto restored = EconomyRuntime::load(saved);
  const auto restoredDiagnostics = restored.diagnostics();
  require(restoredDiagnostics.departmentContribution == diagnostics.departmentContribution,
          "department contribution changed across save/load");
  require(restoredDiagnostics.debtPaymentSchedule == diagnostics.debtPaymentSchedule,
          "debt payment schedule changed across save/load");
}
