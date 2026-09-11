#include "hh/game/EconomyRuntime.h"
#include "hh/game/Overbooking.h"
#include <stdexcept>

using namespace hh::game;
static void require(bool value, const char *message) {
  if (!value) throw std::runtime_error(message);
}

int main() {
  OverbookingSystem policies;
  OverbookingPolicy percentage{"standard", 0, 25'000, 10, 20};
  percentage.allowanceMode = OverbookingAllowanceMode::Percentage;
  percentage.allowanceBasisPoints = 1000; // 10%
  require(policies.setPolicy(percentage).ok, "percentage policy rejected");
  require(policies.allowance("standard", 15, 20) == 2,
          "10 percent allowance did not resolve against physical rooms");
  require(policies.allowance("standard", 15, 35) == 3,
          "percentage allowance did not scale with physical capacity");
  require(policies.allowance("standard", 9, 35) == 0,
          "percentage allowance leaked outside date window");
  const auto savedPolicy = policies.save();
  require(OverbookingSystem::load(savedPolicy).save() == savedPolicy,
          "percentage overbooking policy did not round-trip");

  EconomyRuntime runtime(9191);
  runtime.setPhysicalRoomCapacity("standard", 20);
  require(runtime.setOverbookingPolicy(percentage).ok,
          "runtime rejected percentage overbooking policy");
  require(runtime.revenueManagementSnapshot().inventory.resolvedOverbookingAllowance.at("standard") == 2,
          "runtime did not apply percentage allowance to inventory");
  runtime.setPhysicalRoomCapacity("standard", 30);
  require(runtime.revenueManagementSnapshot().inventory.resolvedOverbookingAllowance.at("standard") == 3,
          "percentage allowance did not refresh when physical capacity changed");
}
