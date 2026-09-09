#include "ShowcaseHotelScene.h"

#include <cmath>
#include <cstddef>

namespace hh::frontend {
namespace {

void addBox(hh::renderer::RenderScene& scene, hh::renderer::Vec3 center,
            hh::renderer::Vec3 size, hh::renderer::Color color,
            hh::renderer::RenderCategory category, int floorId = 0) {
    const hh::renderer::Vec3 half{size.x * 0.5F, size.y * 0.5F, size.z * 0.5F};
    hh::renderer::BoxRenderItem item{};
    item.center = center;
    item.size = size;
    item.color = color;
    item.floorId = floorId;
    item.category = category;
    item.bounds = {center - half, center + half};
    scene.items.push_back(item);
}

bool isPresentationActor(const hh::renderer::BoxRenderItem& item) noexcept {
    return std::fabs(item.size.x - 0.7F) < 0.001F &&
           std::fabs(item.size.y - 2.0F) < 0.001F &&
           std::fabs(item.size.z - 0.7F) < 0.001F;
}

void updateBounds(hh::renderer::BoxRenderItem& item) noexcept {
    const hh::renderer::Vec3 half{
        item.size.x * 0.5F,
        item.size.y * 0.5F,
        item.size.z * 0.5F,
    };
    item.bounds = {item.center - half, item.center + half};
}

}  // namespace

hh::renderer::RenderScene ShowcaseHotelScene::build() {
    using namespace hh::renderer;
    RenderScene scene{};
    scene.activeFloor = 0;
    scene.focusTarget = Vec3{0.0F, 2.0F, 0.0F};

    const Color stone{0.52F, 0.48F, 0.42F, 1.0F};
    const Color warmStone{0.63F, 0.55F, 0.45F, 1.0F};
    const Color glass{0.22F, 0.38F, 0.45F, 0.72F};
    const Color brass{0.72F, 0.59F, 0.36F, 1.0F};
    const Color dark{0.16F, 0.15F, 0.14F, 1.0F};
    const Color green{0.19F, 0.30F, 0.20F, 1.0F};

    addBox(scene, {0.0F, -0.25F, 0.0F}, {36.0F, 0.5F, 24.0F}, stone, RenderCategory::Floor);
    addBox(scene, {0.0F, 2.6F, 9.0F}, {32.0F, 5.2F, 1.0F}, warmStone, RenderCategory::Wall);
    addBox(scene, {-15.5F, 2.6F, 0.0F}, {1.0F, 5.2F, 18.0F}, warmStone, RenderCategory::Wall);
    addBox(scene, {15.5F, 2.6F, 0.0F}, {1.0F, 5.2F, 18.0F}, warmStone, RenderCategory::Wall);

    for (int floor = 1; floor <= 3; ++floor) {
        const float y = 5.0F + static_cast<float>(floor) * 4.2F;
        addBox(scene, {0.0F, y, 6.5F}, {31.0F, 0.45F, 5.0F}, stone, RenderCategory::Floor, floor);
        for (int room = 0; room < 7; ++room) {
            const float x = -12.0F + static_cast<float>(room) * 4.0F;
            addBox(scene, {x, y + 1.8F, 8.4F}, {3.5F, 3.0F, 0.35F}, glass, RenderCategory::Object, floor);
        }
    }

    addBox(scene, {0.0F, 0.55F, -1.5F}, {12.0F, 1.1F, 4.0F}, dark, RenderCategory::Object);
    addBox(scene, {0.0F, 1.25F, -1.5F}, {8.0F, 0.35F, 2.0F}, brass, RenderCategory::Object);
    addBox(scene, {-8.0F, 0.7F, -3.5F}, {4.0F, 1.4F, 3.0F}, dark, RenderCategory::Object);
    addBox(scene, {8.0F, 0.7F, -3.5F}, {4.0F, 1.4F, 3.0F}, dark, RenderCategory::Object);

    for (int i = 0; i < 8; ++i) {
        const float x = -10.5F + static_cast<float>(i) * 3.0F;
        addBox(scene, {x, 1.0F, 2.0F + static_cast<float>(i % 2)}, {0.7F, 2.0F, 0.7F},
               Color{0.56F, 0.43F + 0.03F * static_cast<float>(i % 3), 0.31F, 1.0F}, RenderCategory::Object);
    }

    addBox(scene, {-12.0F, 1.5F, -7.0F}, {1.5F, 3.0F, 1.5F}, green, RenderCategory::Object);
    addBox(scene, {12.0F, 1.5F, -7.0F}, {1.5F, 3.0F, 1.5F}, green, RenderCategory::Object);
    addBox(scene, {0.0F, 1.6F, -9.0F}, {5.0F, 3.2F, 0.5F}, glass, RenderCategory::Object);
    addBox(scene, {0.0F, 0.2F, -14.0F}, {10.0F, 0.4F, 5.0F}, dark, RenderCategory::Object);
    addBox(scene, {0.0F, 1.0F, -14.0F}, {4.0F, 1.5F, 2.0F}, Color{0.18F, 0.20F, 0.22F, 1.0F}, RenderCategory::Object);

    return scene;
}

void ShowcaseHotelScene::updateAmbient(
    hh::renderer::RenderScene& scene,
    float elapsedSeconds) noexcept {
    std::size_t actorIndex = 0;
    for (auto& item : scene.items) {
        if (!isPresentationActor(item)) {
            continue;
        }

        const float index = static_cast<float>(actorIndex);
        const float baseX = -10.5F + index * 3.0F;
        const float baseZ = 2.0F + static_cast<float>(actorIndex % 2U);
        const float phase = elapsedSeconds * 0.35F + index * 0.9F;

        item.center.x = baseX + std::sin(phase) * 0.75F;
        item.center.z = baseZ + std::cos(phase * 0.7F) * 0.45F;
        updateBounds(item);
        ++actorIndex;
    }
}

}  // namespace hh::frontend
