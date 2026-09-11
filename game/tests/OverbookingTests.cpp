#include "hh/game/Overbooking.h"
#include <stdexcept>

using namespace hh::game;
static void require(bool v, const char *m) { if (!v) throw std::runtime_error(m); }

int main() {
  OverbookingSystem system;
  require(system.setPolicy({"standard", 2, 25000}).ok, "valid overbooking policy rejected");
  require(system.allowance("standard") == 2, "overbooking allowance not retained");

  RecoveryContext context;
  context.categoryEquivalentAvailable = false;
  context.freeUpgradeAvailable = true;
  context.paidUpgradeAvailable = true;
  context.guestAcceptsPaidUpgrade = true;
  context.accelerationFeasible = true;
  context.competitorRelocationAvailable = true;
  auto recovery = system.chooseRecovery(context);
  require(recovery.action == RecoveryAction::FreeUpgrade,
          "recovery did not follow category-equivalent/free-upgrade priority");

  context.freeUpgradeAvailable = false;
  recovery = system.chooseRecovery(context);
  require(recovery.action == RecoveryAction::PaidUpgrade,
          "paid accepted upgrade was not selected before acceleration/relocation");

  context.paidUpgradeAvailable = false;
  recovery = system.chooseRecovery(context);
  require(recovery.action == RecoveryAction::AccelerateRoomRecovery,
          "feasible room recovery acceleration was not selected before relocation");

  context.accelerationFeasible = false;
  recovery = system.chooseRecovery(context);
  require(recovery.action == RecoveryAction::CompetitorRelocation && recovery.compensationCents == 25000,
          "competitor relocation/compensation was not deterministic final recovery");
}
