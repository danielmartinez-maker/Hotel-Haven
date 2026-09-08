#include "TestFramework.h"
#include "hh/renderer/Camera.h"
#include "hh/renderer/FloorVisibility.h"
#include "hh/renderer/Occlusion.h"
#include "hh/renderer/SceneComposer.h"

#include <cmath>
#include <cstddef>
#include <limits>

namespace {

hh::renderer::BoxRenderItem makeStressBox(int index, int floorId) {
    using namespace hh::renderer;
    const float x = static_cast<float>(index % 256);
    const float z = static_cast<float>((index / 256) % 256);
    const Vec3 center{x, 1.0f, z};
    const Vec3 size{1.0f, 2.0f, 1.0f};
    const RenderCategory category = static_cast<RenderCategory>(index % 4);
    return BoxRenderItem{
        center,
        size,
        Color{0.8f, 0.8f, 0.8f, 1.0f},
        floorId,
        category,
        Aabb{{x - 0.5f, 0.0f, z - 0.5f}, {x + 0.5f, 2.0f, z + 0.5f}}
    };
}

}  // namespace

TEST_CASE("floor visibility does not overflow on hostile floor ids") {
    using namespace hh::renderer;
    EXPECT_EQ(
        floorVisibility(std::numeric_limits<int>::max(),
                        std::numeric_limits<int>::min(),
                        FloorContextMode::AdjacentContext),
        FloorRenderVisibility::Hidden);
    EXPECT_EQ(
        floorVisibility(std::numeric_limits<int>::min(),
                        std::numeric_limits<int>::max(),
                        FloorContextMode::AdjacentContext),
        FloorRenderVisibility::Hidden);
}

TEST_CASE("camera rejects non-finite control inputs") {
    using namespace hh::renderer;
    OrthoCamera camera;
    camera.setPitchDegrees(60.0f);
    camera.setYawDegrees(90.0f);
    camera.setOrthoHeight(20.0f);
    camera.setAspectRatio(2.0f);

    camera.setPitchDegrees(std::numeric_limits<float>::quiet_NaN());
    camera.setYawDegrees(std::numeric_limits<float>::infinity());
    camera.setOrthoHeight(std::numeric_limits<float>::quiet_NaN());
    camera.setAspectRatio(std::numeric_limits<float>::infinity());

    EXPECT_TRUE(std::isfinite(camera.pitchDegrees()));
    EXPECT_TRUE(std::isfinite(camera.yawDegrees()));
    EXPECT_TRUE(std::isfinite(camera.orthoHeight()));
    EXPECT_TRUE(std::isfinite(camera.aspectRatio()));
    EXPECT_NEAR(camera.pitchDegrees(), 60.0f, 0.0001f);
    EXPECT_NEAR(camera.yawDegrees(), 90.0f, 0.0001f);
    EXPECT_NEAR(camera.orthoHeight(), 20.0f, 0.0001f);
    EXPECT_NEAR(camera.aspectRatio(), 2.0f, 0.0001f);
    EXPECT_NEAR(OrthoCamera::snapYaw90(std::numeric_limits<float>::infinity()), 0.0f, 0.0001f);

    const Vec3 position = camera.worldPosition();
    EXPECT_TRUE(std::isfinite(position.x));
    EXPECT_TRUE(std::isfinite(position.y));
    EXPECT_TRUE(std::isfinite(position.z));
}

TEST_CASE("scene composer handles thirty two thousand items deterministically") {
    using namespace hh::renderer;
    constexpr int kItemCount = 32768;
    RenderScene scene;
    scene.activeFloor = 3;
    scene.items.reserve(kItemCount);

    for (int i = 0; i < kItemCount; ++i) {
        const int floorId = -4 + (i % 16);
        scene.items.push_back(makeStressBox(i, floorId));
    }

    const SceneComposer composer;
    const Vec3 camera{128.0f, 80.0f, -100.0f};
    const auto first = composer.compose(
        scene, FloorContextMode::AdjacentContext, WallRenderMode::FullHeight, camera);
    const auto second = composer.compose(
        scene, FloorContextMode::AdjacentContext, WallRenderMode::FullHeight, camera);

    EXPECT_EQ(first.opaque.size(), static_cast<std::size_t>(2048));
    EXPECT_EQ(first.translucent.size(), static_cast<std::size_t>(4096));
    EXPECT_TRUE(first.wireframe.empty());
    EXPECT_EQ(first.opaque.size(), second.opaque.size());
    EXPECT_EQ(first.translucent.size(), second.translucent.size());

    for (std::size_t i = 0; i < first.opaque.size(); ++i) {
        EXPECT_NEAR(first.opaque[i].item.center.x, second.opaque[i].item.center.x, 0.0f);
        EXPECT_NEAR(first.opaque[i].item.center.z, second.opaque[i].item.center.z, 0.0f);
        EXPECT_EQ(first.opaque[i].cutaway, second.opaque[i].cutaway);
    }
    for (std::size_t i = 0; i < first.translucent.size(); ++i) {
        EXPECT_NEAR(first.translucent[i].item.color.a, 0.25f, 0.0001f);
        EXPECT_NEAR(first.translucent[i].item.center.x, second.translucent[i].item.center.x, 0.0f);
        EXPECT_NEAR(first.translucent[i].item.center.z, second.translucent[i].item.center.z, 0.0f);
    }
}

TEST_CASE("occlusion handles degenerate segments") {
    using namespace hh::renderer;
    const Aabb bounds{{-1.0f, -1.0f, -1.0f}, {1.0f, 1.0f, 1.0f}};
    EXPECT_TRUE(segmentIntersectsAabb({0.0f, 0.0f, 0.0f}, {0.0f, 0.0f, 0.0f}, bounds));
    EXPECT_FALSE(segmentIntersectsAabb({2.0f, 2.0f, 2.0f}, {2.0f, 2.0f, 2.0f}, bounds));
}
