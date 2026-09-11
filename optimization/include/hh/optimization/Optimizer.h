#pragma once

#include <cstdint>

#include "hh/optimization/Types.h"

namespace hh::optimization {

std::uint64_t snapshotFingerprint(const OptimizerSnapshot& snapshot) noexcept;

class IStaffOptimizer {
public:
    virtual ~IStaffOptimizer() = default;
    virtual SchedulerPlan optimize(const OptimizerSnapshot& snapshot) const = 0;
};

class DeterministicFallbackOptimizer final : public IStaffOptimizer {
public:
    SchedulerPlan optimize(const OptimizerSnapshot& snapshot) const override;
};

}  // namespace hh::optimization
