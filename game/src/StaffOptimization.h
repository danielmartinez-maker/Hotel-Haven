#pragma once

#include "hh/optimization/Types.h"

namespace hh::game::detail {

struct PlanResolution {
  hh::optimization::SchedulerPlan plan;
  bool valid{};
  bool acceptedProposed{};
  bool fellBack{};
};

PlanResolution resolveStaffPlan(
    const hh::optimization::OptimizerSnapshot &snapshot,
    const hh::optimization::SchedulerPlan *proposed = nullptr);

} // namespace hh::game::detail
