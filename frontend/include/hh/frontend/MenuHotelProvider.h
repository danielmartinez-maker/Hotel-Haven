#pragma once

#include <cstdint>
#include <optional>
#include <string>

namespace hh::frontend {

struct MenuPropertySummary {
    std::string hotelName;
    std::string city;
    std::string country;
    int starRating{};
    std::uint32_t currentDay{};
    std::optional<float> occupancy;
    std::optional<float> guestSatisfaction;
    std::optional<std::int64_t> cashMinorUnits;
    std::string currencyCode;
    std::optional<std::uint32_t> roomCount;
    std::uint64_t lastPlayedUtc{};
};

struct MenuScenePresentation {
    std::string sceneAssetId;
    std::uint32_t snapshotVersion{};
    bool compatible{true};
};

struct MenuCameraAnchor {
    float targetX{};
    float targetY{};
    float targetZ{};
    float yawDegrees{};
    float pitchDegrees{55.0F};
    float orthoHeight{36.0F};
};

struct MenuHotelSnapshot {
    MenuPropertySummary summary;
    MenuScenePresentation scene;
    MenuCameraAnchor camera;
};

class IMenuHotelProvider {
public:
    virtual ~IMenuHotelProvider() = default;
    [[nodiscard]] virtual std::optional<MenuHotelSnapshot> latestHotel() = 0;
};

}  // namespace hh::frontend
