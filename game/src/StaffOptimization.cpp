#include "StaffOptimization.h"

#include "hh/optimization/Optimizer.h"
#include "hh/optimization/PlanValidator.h"

#include <utility>

namespace hh::game::detail {

PlanResolution resolveStaffPlan(
    const hh::optimization::OptimizerSnapshot &snapshot,
    const hh::optimization::SchedulerPlan *proposed) {
  if (proposed) {
    const auto validation = hh::optimization::validatePlan(snapshot, *proposed);
    if (validation.ok)
      return {*proposed, true, true, false};
  }

  hh::optimization::DeterministicFallbackOptimizer optimizer;
  auto nativePlan = optimizer.optimize(snapshot);
  const auto nativeValidation = hh::optimization::validatePlan(snapshot, nativePlan);
  return {std::move(nativePlan), nativeValidation.ok, false, proposed != nullptr};
}

} // namespace hh::game::detail
