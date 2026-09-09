#include "hh/frontend/MainMenuView.h"

#include <algorithm>
#include <cmath>
#include <iomanip>
#include <locale>
#include <sstream>

namespace hh::frontend {
namespace {

constexpr float kReferenceWidth = 1920.0F;
constexpr float kReferenceHeight = 1080.0F;
constexpr char kMissingValue[] = "\xE2\x80\x94";

std::string trimFixed(double value, int precision) {
    std::ostringstream stream;
    stream.imbue(std::locale::classic());
    stream << std::fixed << std::setprecision(precision) << value;
    std::string text = stream.str();
    const auto decimal = text.find('.');
    if (decimal != std::string::npos) {
        while (!text.empty() && text.back() == '0') {
            text.pop_back();
        }
        if (!text.empty() && text.back() == '.') {
            text.pop_back();
        }
    }
    return text;
}

std::string currencyPrefix(std::string_view code) {
    if (code == "EUR") {
        return "\xE2\x82\xAC";
    }
    if (code == "USD") {
        return "$";
    }
    if (code == "GBP") {
        return "\xC2\xA3";
    }
    if (code == "MXN") {
        return "MX$";
    }
    if (code.empty()) {
        return {};
    }
    return std::string(code) + ' ';
}

}  // namespace

std::string DefaultMenuNumberFormatter::percentage(float normalizedValue) const {
    const float clamped = std::clamp(normalizedValue, 0.0F, 1.0F);
    return std::to_string(static_cast<int>(std::lround(clamped * 100.0F))) + '%';
}

std::string DefaultMenuNumberFormatter::cash(
    std::int64_t minorUnits,
    std::string_view currencyCode) const {
    const double majorUnits = static_cast<double>(minorUnits) / 100.0;
    const double magnitude = std::fabs(majorUnits);
    std::string amount;
    if (magnitude >= 1'000'000.0) {
        amount = trimFixed(majorUnits / 1'000'000.0, 2) + 'M';
    } else if (magnitude >= 1'000.0) {
        amount = trimFixed(majorUnits / 1'000.0, 1) + 'K';
    } else {
        amount = trimFixed(majorUnits, 2);
    }
    return currencyPrefix(currencyCode) + amount;
}

std::string DefaultMenuNumberFormatter::unsignedValue(std::uint64_t value) const {
    return std::to_string(value);
}

MainMenuView::MainMenuView() noexcept : formatter_(&defaultFormatter_) {}

MainMenuView::MainMenuView(const IMenuNumberFormatter& formatter) noexcept : formatter_(&formatter) {}

PropertyCardText MainMenuView::formatProperty(const MenuPropertySummary& summary) const {
    PropertyCardText result;
    result.hotelName = summary.hotelName;
    if (!summary.city.empty() && !summary.country.empty()) {
        result.location = summary.city + ", " + summary.country;
    } else if (!summary.city.empty()) {
        result.location = summary.city;
    } else {
        result.location = summary.country;
    }
    result.visualStars = std::clamp(summary.starRating, 0, 5);
    result.day = formatter_->unsignedValue(summary.currentDay);
    result.occupancy = summary.occupancy.has_value()
                           ? formatter_->percentage(*summary.occupancy)
                           : kMissingValue;
    result.satisfaction = summary.guestSatisfaction.has_value()
                              ? formatter_->percentage(*summary.guestSatisfaction)
                              : kMissingValue;
    result.cash = summary.cashMinorUnits.has_value()
                      ? formatter_->cash(*summary.cashMinorUnits, summary.currencyCode)
                      : kMissingValue;
    result.rooms = summary.roomCount.has_value()
                       ? formatter_->unsignedValue(*summary.roomCount)
                       : kMissingValue;
    return result;
}

LayoutMetrics MainMenuView::layout(
    float physicalWidth,
    float physicalHeight,
    float requestedUiScale) const noexcept {
    LayoutMetrics result;
    result.viewportWidth = std::max(physicalWidth, 1.0F);
    result.viewportHeight = std::max(physicalHeight, 1.0F);
    result.logicalScale = std::min(result.viewportWidth / kReferenceWidth, result.viewportHeight / kReferenceHeight);
    result.uiScale = std::clamp(requestedUiScale, 0.90F, 1.50F);

    result.safeZoneWidth = kReferenceWidth * result.logicalScale;
    result.safeZoneHeight = kReferenceHeight * result.logicalScale;
    result.safeZoneLeft = (result.viewportWidth - result.safeZoneWidth) * 0.5F;
    result.safeZoneTop = (result.viewportHeight - result.safeZoneHeight) * 0.5F;

    const float scaledUi = result.logicalScale * result.uiScale;
    result.navigationLeft = result.safeZoneLeft + 72.0F * scaledUi;
    result.navigationTop = result.safeZoneTop + 300.0F * scaledUi;
    result.navigationWidth = 360.0F * scaledUi;
    result.propertyCardWidth = 320.0F * scaledUi;
    result.propertyCardLeft = result.safeZoneLeft + result.safeZoneWidth - 64.0F * scaledUi - result.propertyCardWidth;
    result.propertyCardTop = result.safeZoneTop + result.safeZoneHeight * 0.48F - 170.0F * scaledUi;
    result.versionLeft = result.safeZoneLeft + 72.0F * scaledUi;
    result.versionBottom = result.safeZoneTop + result.safeZoneHeight - 48.0F * scaledUi;
    return result;
}

}  // namespace hh::frontend
