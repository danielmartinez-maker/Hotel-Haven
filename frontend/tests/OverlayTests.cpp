#include "TestFramework.h"
#include "hh/frontend/OverlayModel.h"

using namespace hh::frontend;

TEST_CASE("All required overlays expose exact legends and non-color semantics") {
    const auto descriptors = OverlayModel::requiredDescriptors();
    EXPECT_EQ(descriptors.size(), static_cast<std::size_t>(17));
    for (const auto& descriptor : descriptors) {
        EXPECT_FALSE(descriptor.label.empty());
        EXPECT_FALSE(descriptor.unit.empty());
        EXPECT_TRUE(descriptor.maxValue >= descriptor.minValue);
        EXPECT_FALSE(descriptor.lowSemantic.empty());
        EXPECT_FALSE(descriptor.highSemantic.empty());
    }
}

TEST_CASE("Overlay hover returns authoritative exact value") {
    OverlaySnapshot snapshot;
    snapshot.id = OverlayId::Temperature;
    snapshot.unit = "C";
    snapshot.minValue = 16;
    snapshot.maxValue = 34;
    snapshot.samples.push_back({55, 22450, "22.45 C", "Comfortable"});
    OverlayModel model;
    model.bind(snapshot);
    const auto sample = model.sampleFor(55);
    EXPECT_TRUE(sample.has_value());
    EXPECT_EQ(sample->scaledValue, static_cast<std::int64_t>(22450));
    EXPECT_EQ(sample->exactText, std::string("22.45 C"));
}
