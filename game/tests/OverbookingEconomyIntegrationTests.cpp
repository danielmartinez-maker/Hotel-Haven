#include "hh/game/EconomyRuntime.h"
#include <stdexcept>

using namespace hh::game;
static void require(bool v, const char *m) { if (!v) throw std::runtime_error(m); }

int main() {
  EconomyRuntime runtime(6060, 1'000'000);
  require(runtime.setOverbookingPolicy({"standard", 2, 25'000}).ok,
          "overbooking policy rejected");

  RecoveryContext relocation;
  relocation.roomCategory = "standard";
  relocation.competitorRelocationAvailable = true;
  const auto before = runtime.financialSnapshot().economics;
  const auto relocated = runtime.resolveOverbooking(relocation, 9001);
  require(relocated.action == RecoveryAction::CompetitorRelocation &&
              relocated.compensationCents == 25'000,
          "competitor relocation did not retain configured compensation");
  require(!relocated.severeExperienceEvent,
          "successful relocation incorrectly emitted unresolved severe experience event");
  const auto after = runtime.financialSnapshot().economics;
  require(after.cashCents == before.cashCents - 25'000 &&
              after.operatingCostCents == before.operatingCostCents + 25'000,
          "relocation compensation did not post through exact-cent ledger");

  RecoveryContext unresolved;
  unresolved.roomCategory = "standard";
  const auto failed = runtime.resolveOverbooking(unresolved, 9002);
  require(failed.action == RecoveryAction::Unresolved && failed.severeExperienceEvent,
          "unresolved overbooking did not expose severe guest-experience handoff");
  require(failed.experienceEventCode == "OVERBOOKING_UNRESOLVED_SEVERE",
          "unresolved overbooking experience event lacks stable reason code");

  const auto saved = runtime.save();
  require(EconomyRuntime::load(saved).save() == saved,
          "overbooking compensation ledger state did not round-trip");
}
