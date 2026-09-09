#pragma once

#include <cstdint>
#include <string>
#include <string_view>

#include "hh/frontend/MenuHotelProvider.h"

namespace hh::frontend {

struct PropertyCardText {
    std::string hotelName;
    std::string location;
    int visualStars{};
    std::string day;
    std::string occupancy;
    std::string satisfaction;
    std::string cash;
    std::string rooms;
};

struct LayoutMetrics {
    float viewportWidth{};
    float viewportHeight{};
    float logicalScale{1.0F};
    float uiScale{1.0F};
    float safeZoneLeft{};
    float safeZoneTop{};
    float safeZoneWidth{};
    float safeZoneHeight{};
    float navigationLeft{};
    float navigationTop{};
    float navigationWidth{};
    float propertyCardLeft{};
    float propertyCardTop{};
    float propertyCardWidth{};
    float versionLeft{};
    float versionBottom{};
};

class IMenuNumberFormatter {
public:
    virtual ~IMenuNumberFormatter() = default;
    [[nodiscard]] virtual std::string percentage(float normalizedValue) const = 0;
    [[nodiscard]] virtual std::string cash(std::int64_t minorUnits, std::string_view currencyCode) const = 0;
    [[nodiscard]] virtual std::string unsignedValue(std::uint64_t value) const = 0;
};

class DefaultMenuNumberFormatter final : public IMenuNumberFormatter {
public:
    [[nodiscard]] std::string percentage(float normalizedValue) const override;
    [[nodiscard]] std::string cash(std::int64_t minorUnits, std::string_view currencyCode) const override;
    [[nodiscard]] std::string unsignedValue(std::uint64_t value) const override;
};

class MainMenuView {
public:
    MainMenuView() noexcept;
    explicit MainMenuView(const IMenuNumberFormatter& formatter) noexcept;

    [[nodiscard]] PropertyCardText formatProperty(const MenuPropertySummary& summary) const;
    [[nodiscard]] LayoutMetrics layout(float physicalWidth, float physicalHeight, float requestedUiScale) const noexcept;

private:
    DefaultMenuNumberFormatter defaultFormatter_;
    const IMenuNumberFormatter* formatter_{};
};

}  // namespace hh::frontend
