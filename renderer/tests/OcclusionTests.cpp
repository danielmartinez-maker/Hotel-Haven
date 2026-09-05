#include "TestFramework.h"
#include "hh/renderer/Occlusion.h"

TEST_CASE("segment intersects foreground wall bounds") {
    using namespace hh::renderer;
    const Vec3 camera{0.0f, 5.0f, -10.0f};
    const Vec3 target{0.0f, 1.0f, 0.0f};
    const Aabb wall{{-1.0f, 0.0f, -5.5f}, {1.0f, 3.0f, -4.5f}};
    EXPECT_TRUE(segmentIntersectsAabb(camera, target, wall));
}

TEST_CASE("segment misses off-axis wall bounds") {
    using namespace hh::renderer;
    const Vec3 camera{0.0f, 5.0f, -10.0f};
    const Vec3 target{0.0f, 1.0f, 0.0f};
    const Aabb wall{{9.0f, 0.0f, -5.5f}, {11.0f, 3.0f, -4.5f}};
    EXPECT_FALSE(segmentIntersectsAabb(camera, target, wall));
}
