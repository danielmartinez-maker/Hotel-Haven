#include "hh/renderer/Occlusion.h"

#include <algorithm>
#include <cmath>

namespace hh::renderer {

bool segmentIntersectsAabb(Vec3 start, Vec3 end, const Aabb& bounds) noexcept {
    constexpr float kParallelEpsilon = 1.0e-6f;
    float tMin = 0.0f;
    float tMax = 1.0f;
    const Vec3 direction = end - start;

    const auto testAxis = [&](float origin, float delta, float slabMin, float slabMax) noexcept {
        if (std::fabs(delta) < kParallelEpsilon) {
            return origin >= slabMin && origin <= slabMax;
        }

        float t1 = (slabMin - origin) / delta;
        float t2 = (slabMax - origin) / delta;
        if (t1 > t2) {
            std::swap(t1, t2);
        }
        tMin = std::max(tMin, t1);
        tMax = std::min(tMax, t2);
        return tMin <= tMax;
    };

    return testAxis(start.x, direction.x, bounds.min.x, bounds.max.x) &&
           testAxis(start.y, direction.y, bounds.min.y, bounds.max.y) &&
           testAxis(start.z, direction.z, bounds.min.z, bounds.max.z);
}

}  // namespace hh::renderer
