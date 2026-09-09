#pragma once

#include <cstdint>
#include <string>

#include "hh/optimization/Types.h"

namespace hh::optimization {

struct MilpRequestOptions {
    std::int32_t bucketMinutes{5};
    std::int32_t horizonBuckets{12};
};

std::string buildMilpRequestJson(const OptimizerSnapshot& snapshot, MilpRequestOptions options = {});

}  // namespace hh::optimization
