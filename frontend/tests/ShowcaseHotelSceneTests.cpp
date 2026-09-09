#include "TestFramework.h"

#include <cmath>

#include "ShowcaseHotelScene.h"

TEST_CASE("showcase hotel ambient update moves presentation actors without changing scene size") {
    auto scene = hh::frontend::ShowcaseHotelScene::build();
    const auto originalSize = scene.items.size();

    bool foundActor = false;
    hh::renderer::Vec3 originalActorCenter{};
    for (const auto& item : scene.items) {
        if (std::fabs(item.size.x - 0.7F) < 0.001F &&
            std::fabs(item.size.y - 2.0F) < 0.001F &&
            std::fabs(item.size.z - 0.7F) < 0.001F) {
            originalActorCenter = item.center;
            foundActor = true;
            break;
        }
    }
    EXPECT_TRUE(foundActor);

    hh::frontend::ShowcaseHotelScene::updateAmbient(scene, 7.0F);
    EXPECT_EQ(scene.items.size(), originalSize);

    bool movedActor = false;
    for (const auto& item : scene.items) {
        if (std::fabs(item.size.x - 0.7F) < 0.001F &&
            std::fabs(item.size.y - 2.0F) < 0.001F &&
            std::fabs(item.size.z - 0.7F) < 0.001F) {
            if (std::fabs(item.center.x - originalActorCenter.x) > 0.01F ||
                std::fabs(item.center.z - originalActorCenter.z) > 0.01F) {
                movedActor = true;
                break;
            }
        }
    }
    EXPECT_TRUE(movedActor);
}

TEST_CASE("showcase hotel ambient update is deterministic for fixed time") {
    auto first = hh::frontend::ShowcaseHotelScene::build();
    auto second = hh::frontend::ShowcaseHotelScene::build();
    hh::frontend::ShowcaseHotelScene::updateAmbient(first, 12.5F);
    hh::frontend::ShowcaseHotelScene::updateAmbient(second, 12.5F);
    EXPECT_EQ(first.items.size(), second.items.size());
    for (std::size_t i = 0; i < first.items.size(); ++i) {
        EXPECT_NEAR(first.items[i].center.x, second.items[i].center.x, 0.0001F);
        EXPECT_NEAR(first.items[i].center.y, second.items[i].center.y, 0.0001F);
        EXPECT_NEAR(first.items[i].center.z, second.items[i].center.z, 0.0001F);
    }
}
