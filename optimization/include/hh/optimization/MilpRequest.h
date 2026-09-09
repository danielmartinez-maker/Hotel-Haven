#pragma once

#include <cstdint>
#include <string>

#include "hh/optimization/Types.h"

namespace hh::optimization {

struct MilpRequestOptions {
    std::int32_t bucketMinutes{kLivePlanningBucketMinutes};
    std::int32_t horizonBuckets{kLivePlanningHorizonBuckets};
};

std::string buildMilpRequestJson(const OptimizerSnapshot& snapshot, MilpRequestOptions options = {});

}  // namespace hh::optimization
