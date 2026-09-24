#pragma once

#include <string>
#include <vector>

#include "hh/optimization/Types.h"

namespace hh::optimization {

struct ValidationResult {
    bool ok{true};
    std::vector<std::string> errors;
};

ValidationResult validatePlan(const OptimizerSnapshot& snapshot, const SchedulerPlan& plan);

}  // namespace hh::optimization
