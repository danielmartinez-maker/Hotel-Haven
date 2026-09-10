#include "TestFramework.h"
#include "StressScale.h"
#include "hh/renderer/Camera.h"
#include "hh/renderer/FloorVisibility.h"
#include "hh/renderer/SceneComposer.h"

#include <algorithm>
#include <cmath>
#include <cstdint>
#include <stdexcept>

namespace {
using namespace hh::renderer;

std::size_t cumulativeInstances() {
    const auto value = stress_test::scaleFromEnvironment();
    if (value == "pr") return 250'000;
    if (value == "extended") return 2'500'000;
    if (value == "exhaustive") return 10'000'000;
    throw std::runtime_error("HH_STRESS_SCALE must be pr, extended, or exhaustive");
}

BoxRenderItem makeBox(std::size_t index, int floor) {
    const float x = static_cast<float>(index % 64);
    const float z = static_cast<float>((index / 64) % 64);
    const float y = static_cast<float>(floor * 3 + 1);
    const Vec3 center{x, y, z};
    const Vec3 size{1.0f, 2.0f, 1.0f};
    return BoxRenderItem{center,
                         size,
                         Color{0.8f, 0.8f, 0.8f, 1.0f},
                         floor,
                         index % 7 == 0 ? RenderCategory::Wall : RenderCategory::Object,
                         Aabb{{x - 0.5f, y - 1.0f, z - 0.5f},
                              {x + 0.5f, y + 1.0f, z + 0.5f}}};
}

MeshRenderItem makeMesh(std::size_t index, int floor) {
    const float x = static_cast<float>((index * 3) % 64);
    const float z = static_cast<float>((index * 5) % 64);
    const float y = static_cast<float>(floor * 3 + 1);
    return MeshRenderItem{AssetHandle{static_cast<std::uint32_t>(1 + index % 500)},
                          MeshTransform{{x, y, z}, {1.0f, 1.0f, 1.0f}, 0.0f},
                          Color{1.0f, 1.0f, 1.0f, 1.0f},
                          floor,
                          RenderCategory::Object,
                          Aabb{{x - 0.5f, y - 0.5f, z - 0.5f},
                               {x + 0.5f, y + 0.5f, z + 0.5f}},
                          index % 11 == 0};
}

std::size_t composedCount(const ComposedScene& scene) {
    return scene.opaque.size() + scene.translucent.size() + scene.wireframe.size() +
           scene.opaqueMeshes.size() + scene.translucentMeshes.size() +
           scene.wireframeMeshes.size();
}
}  // namespace

TEST_CASE("renderer core sustains large deterministic scene rebuild churn") {
    const std::size_t total = cumulativeInstances();
    constexpr std::size_t kPerScene = 1'000;
    const std::size_t cycles = (total + kPerScene - 1) / kPerScene;
    SceneComposer composer;
    std::size_t processed = 0;

    for (std::size_t cycle = 0; cycle < cycles; ++cycle) {
        RenderScene scene;
        scene.activeFloor = static_cast<int>(cycle % 5);
        scene.focusTarget = Vec3{16.0f, static_cast<float>(scene.activeFloor * 3 + 1), 16.0f};
        const std::size_t count = std::min(kPerScene, total - processed);
        scene.items.reserve(count / 2 + 1);
        scene.meshes.reserve(count / 2 + 1);
        for (std::size_t i = 0; i < count; ++i) {
            const int floor = static_cast<int>((processed + i) % 5);
            if ((i & 1U) == 0U)
                scene.items.push_back(makeBox(processed + i, floor));
            else
                scene.meshes.push_back(makeMesh(processed + i, floor));
        }

        const auto normal = composer.compose(scene, FloorContextMode::Normal,
                                             cycle % 2 == 0 ? WallRenderMode::FullHeight
                                                            : WallRenderMode::Blueprint,
                                             {16.0f, 40.0f, -40.0f});
        const auto adjacent = composer.compose(scene, FloorContextMode::AdjacentContext,
                                               WallRenderMode::Cutaway,
                                               {16.0f, 40.0f, -40.0f});
        EXPECT_TRUE(composedCount(normal) <= count);
        EXPECT_TRUE(composedCount(adjacent) <= count);
        EXPECT_TRUE(composedCount(adjacent) >= composedCount(normal));
        for (const auto& mesh : normal.opaqueMeshes)
            EXPECT_TRUE(mesh.item.asset.value >= 1u && mesh.item.asset.value <= 500u);
        for (const auto& mesh : normal.translucentMeshes)
            EXPECT_TRUE(mesh.item.asset.value >= 1u && mesh.item.asset.value <= 500u);
        processed += count;
    }

    EXPECT_EQ(processed, total);
}

TEST_CASE("floor visibility churn never returns an invalid visibility state") {
    constexpr int kIterations = 500'000;
    for (int i = 0; i < kIterations; ++i) {
        const int itemFloor = -4 + (i % 16);
        const int activeFloor = -4 + ((i / 7) % 16);
        const auto normal = floorVisibility(itemFloor, activeFloor, FloorContextMode::Normal);
        const auto adjacent = floorVisibility(itemFloor, activeFloor, FloorContextMode::AdjacentContext);
        EXPECT_TRUE(normal == FloorRenderVisibility::Hidden ||
                    normal == FloorRenderVisibility::Full ||
                    normal == FloorRenderVisibility::TranslucentShell);
        EXPECT_TRUE(adjacent == FloorRenderVisibility::Hidden ||
                    adjacent == FloorRenderVisibility::Full ||
                    adjacent == FloorRenderVisibility::TranslucentShell);
        if (itemFloor == activeFloor) {
            EXPECT_EQ(normal, FloorRenderVisibility::Full);
            EXPECT_EQ(adjacent, FloorRenderVisibility::Full);
        } else {
            EXPECT_EQ(normal, FloorRenderVisibility::Hidden);
        }
    }
}

TEST_CASE("orthographic camera remains finite through extreme valid churn") {
    OrthoCamera camera;
    for (int i = 0; i < 100'000; ++i) {
        const float yaw = static_cast<float>((i * 90) % 1440 - 720);
        const float pitch = 20.0f + static_cast<float>(i % 60);
        const float height = 4.0f + static_cast<float>(i % 196);
        const float aspect = 0.5f + static_cast<float>(i % 300) / 100.0f;
        camera.setTarget({static_cast<float>((i % 2048) - 1024),
                          static_cast<float>(i % 64),
                          static_cast<float>(((i * 3) % 2048) - 1024)});
        camera.setYawDegrees(yaw);
        camera.setPitchDegrees(pitch);
        camera.setOrthoHeight(height);
        camera.setAspectRatio(aspect);
        if ((i & 7) == 0)
            camera.rotateSnapped((i % 9) - 4);
        const auto position = camera.worldPosition();
        EXPECT_TRUE(std::isfinite(position.x));
        EXPECT_TRUE(std::isfinite(position.y));
        EXPECT_TRUE(std::isfinite(position.z));
        EXPECT_TRUE(std::isfinite(camera.orthoHeight()));
        EXPECT_TRUE(std::isfinite(camera.aspectRatio()));
    }
}
