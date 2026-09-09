#include "hh/game/Simulation.h"
#include "hh/game/Workforce.h"

#include <iostream>
#include <stdexcept>

using namespace hh::game;

static void require(bool value, const char *message) {
  if (!value)
    throw std::runtime_error(message);
}

static const PersonView &employee(const SimulationView &view, EntityId id) {
  for (const auto &person : view.people)
    if (person.id == id)
      return person;
  throw std::runtime_error("employee missing");
}

static void applicant_pool_is_deterministic_for_campaign_seed_and_day() {
  auto a = Simulation::tutorial(99);
  auto b = Simulation::tutorial(99);

  const auto applicantsA = a.applicants();
  const auto applicantsB = b.applicants();
  require(!applicantsA.empty(), "applicant pool was empty");
  require(applicantsA == applicantsB,
          "same campaign seed/day produced different applicants");
}

static void hiring_deducts_configured_onboarding_cost_and_creates_contract() {
  auto sim = Simulation::tutorial(101);
  require(sim.loadDefinitions(R"({"onboardingCostCents":12345})").ok,
          "onboarding configuration rejected");

  const auto applicants = sim.applicants();
  require(!applicants.empty(), "no applicant available to hire");
  const auto beforeCash = sim.view().economy.cashCents;

  const auto result = sim.hireApplicant(applicants.front().id);
  require(result.ok, "valid applicant hire was rejected");

  const auto view = sim.view();
  const auto &hired = employee(view, result.employeeId);
  require(hired.contract.hourlyWageCents > 0,
          "hired employee contract has no hourly wage");
  require(hired.contract.onboardingCostCents == 12345,
          "configured onboarding cost was not persisted on contract");
  require(view.economy.cashCents == beforeCash - 12345,
          "configured onboarding cost was not deducted exactly");
}

int main() {
  try {
    applicant_pool_is_deterministic_for_campaign_seed_and_day();
    hiring_deducts_configured_onboarding_cost_and_creates_contract();
  } catch (const std::exception &e) {
    std::cerr << "FAIL: " << e.what() << '\n';
    return 1;
  }
  std::cout << "Workforce tests passed\n";
  return 0;
}
