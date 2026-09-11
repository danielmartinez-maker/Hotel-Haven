#include "hh/game/MarketDemand.h"

#include "hh/assets/Json.h"

#include <cmath>
#include <limits>
#include <set>
#include <stdexcept>
#include <string>

namespace hh::game {
namespace {

MarketSegment segmentFromId(std::string_view id) {
  if (id == "budget_leisure") return MarketSegment::BudgetLeisure;
  if (id == "business") return MarketSegment::Business;
  if (id == "executive_business") return MarketSegment::ExecutiveBusiness;
  if (id == "couple_leisure") return MarketSegment::CoupleLeisure;
  if (id == "family_leisure") return MarketSegment::FamilyLeisure;
  if (id == "luxury_leisure") return MarketSegment::LuxuryLeisure;
  if (id == "conference_group") return MarketSegment::ConferenceGroup;
  if (id == "airport_transit") return MarketSegment::AirportTransit;
  if (id == "wellness") return MarketSegment::Wellness;
  throw std::invalid_argument("unknown market segment id");
}

int integerValue(const hh::assets::JsonValue &value, int minimum, int maximum) {
  if (!value.is_number())
    throw std::invalid_argument("market definition value is not numeric");
  const double number = value.as_number();
  if (!std::isfinite(number) || number < static_cast<double>(minimum) ||
      number > static_cast<double>(maximum) || std::floor(number) != number)
    throw std::invalid_argument("market definition integer out of range");
  return static_cast<int>(number);
}

std::int64_t int64Value(const hh::assets::JsonValue &value,
                        std::int64_t minimum,
                        std::int64_t maximum) {
  if (!value.is_number())
    throw std::invalid_argument("market definition value is not numeric");
  const double number = value.as_number();
  if (!std::isfinite(number) || number < static_cast<double>(minimum) ||
      number > static_cast<double>(maximum) || std::floor(number) != number)
    throw std::invalid_argument("market definition integer out of range");
  return static_cast<std::int64_t>(number);
}

const hh::assets::JsonValue *findEither(const hh::assets::JsonValue &object,
                                        std::string_view first,
                                        std::string_view second) {
  if (const auto *value = object.find(first))
    return value;
  return object.find(second);
}

int nestedMedian(const hh::assets::JsonValue &segment,
                 std::string_view directKey,
                 std::string_view objectKey,
                 int minimum,
                 int maximum,
                 int fallback) {
  if (const auto *direct = segment.find(directKey))
    return integerValue(*direct, minimum, maximum);
  if (const auto *object = segment.find(objectKey)) {
    if (!object->is_object())
      throw std::invalid_argument("market definition distribution is not an object");
    return integerValue(object->at("median"), minimum, maximum);
  }
  return fallback;
}

std::int64_t nestedBudget(const hh::assets::JsonValue &segment,
                          std::int64_t fallback) {
  if (const auto *direct = segment.find("base_budget_cents"))
    return int64Value(*direct, 1, std::numeric_limits<std::int64_t>::max());
  if (const auto *object = segment.find("budget_cents")) {
    if (!object->is_object())
      throw std::invalid_argument("market budget distribution is not an object");
    return int64Value(object->at("median"), 1,
                      std::numeric_limits<std::int64_t>::max());
  }
  return fallback;
}

SegmentChoiceWeights parseChoiceWeights(const hh::assets::JsonValue &value) {
  if (!value.is_object())
    throw std::invalid_argument("market choice weights are not an object");
  SegmentChoiceWeights weights;
  weights.priceBasisPoints = integerValue(value.at("price"), 0, 10000);
  weights.reputationBasisPoints = integerValue(value.at("reputation"), 0, 10000);
  weights.amenityBasisPoints = integerValue(value.at("amenity"), 0, 10000);
  weights.locationBasisPoints = integerValue(value.at("location"), 0, 10000);
  weights.starBasisPoints = integerValue(value.at("star"), 0, 10000);
  weights.roomBasisPoints = integerValue(value.at("room"), 0, 10000);
  weights.brandBasisPoints = integerValue(value.at("brand"), 0, 10000);
  if (weights.totalBasisPoints() != 10000)
    throw std::invalid_argument("market choice weights do not sum to one");
  return weights;
}

} // namespace

MarketDefinitionLoadResult MarketDemandSystem::loadDefinitions(
    std::string_view jsonText) {
  try {
    const auto root = hh::assets::parse_json(jsonText);
    if (!root.is_object())
      return {false, "MARKET_DEFINITIONS_ROOT_NOT_OBJECT"};

    if (const auto *schemaVersion = root.find("schema_version")) {
      if (integerValue(*schemaVersion, 2, 2) != 2)
        return {false, "UNSUPPORTED_MARKET_DEFINITIONS_VERSION"};
    } else if (const auto *schema = root.find("schema")) {
      if (!schema->is_string() || schema->as_string() != "hotel-haven.market-segments.v2")
        return {false, "UNSUPPORTED_MARKET_DEFINITIONS_VERSION"};
    }

    auto candidateProfiles = demandProfiles_;
    int candidateTemperature = choiceTemperatureBasisPoints_;
    if (const auto *temperature = root.find("choice_temperature_basis_points"))
      candidateTemperature = integerValue(*temperature, 1, 100000);

    const auto *segments = root.find("segments");
    if (!segments || !segments->is_array())
      return {false, "MARKET_DEFINITIONS_SEGMENTS_REQUIRED"};

    std::set<MarketSegment> seen;
    for (const auto &segmentJson : segments->as_array()) {
      if (!segmentJson.is_object())
        throw std::invalid_argument("market segment definition is not an object");
      const auto &id = segmentJson.at("id");
      if (!id.is_string())
        throw std::invalid_argument("market segment id is not a string");
      const auto segment = segmentFromId(id.as_string());
      if (!seen.insert(segment).second)
        throw std::invalid_argument("duplicate market segment definition");

      auto profileIt = candidateProfiles.find(segment);
      if (profileIt == candidateProfiles.end())
        throw std::invalid_argument("market segment has no baseline profile");
      auto profile = profileIt->second;

      if (const auto *base = segmentJson.find("base_daily_demand")) {
        if (!base->is_number() || !std::isfinite(base->as_number()) ||
            base->as_number() < 0.0 || base->as_number() > 1000000.0)
          throw std::invalid_argument("invalid base daily demand");
        profile.baseDailyDemand = base->as_number();
      }
      profile.baseBudgetCents = nestedBudget(segmentJson, profile.baseBudgetCents);
      profile.medianLeadTimeDays = nestedMedian(
          segmentJson, "median_lead_time_days", "lead_time_days", 0, 3650,
          profile.medianLeadTimeDays);
      profile.medianStayNights = nestedMedian(
          segmentJson, "median_stay_nights", "stay_nights", 1, 3650,
          profile.medianStayNights);

      if (const auto *elasticity = segmentJson.find("price_elasticity_bp"))
        profile.priceElasticityBasisPoints = integerValue(*elasticity, 0, 100000);
      if (const auto *amenity = segmentJson.find("amenity_sensitivity_bp"))
        profile.amenitySensitivityBasisPoints = integerValue(*amenity, 0, 100000);
      if (const auto *cancellation = findEither(
              segmentJson, "cancellation_rate_bp", "cancellation_bp"))
        profile.cancellationBasisPoints = integerValue(*cancellation, 0, 10000);
      if (const auto *noShow = findEither(segmentJson, "no_show_rate_bp", "no_show_bp"))
        profile.noShowBasisPoints = integerValue(*noShow, 0, 10000);

      if (const auto *weekday = segmentJson.find("weekday_multiplier_bp")) {
        if (!weekday->is_array() || weekday->as_array().size() != 7)
          throw std::invalid_argument("weekday profile must contain seven values");
        for (std::size_t i = 0; i < 7; ++i)
          profile.weekdayMultiplierBasisPoints[i] =
              integerValue(weekday->as_array()[i], 0, 100000);
      }
      if (const auto *weights = segmentJson.find("choice_weights_basis_points"))
        profile.choiceWeights = parseChoiceWeights(*weights);

      MarketDemandSystem validator(seed_);
      validator.setSegmentDemandProfile(profile);
      candidateProfiles[segment] = profile;
    }

    demandProfiles_ = std::move(candidateProfiles);
    choiceTemperatureBasisPoints_ = candidateTemperature;
    return {true, "OK"};
  } catch (const std::exception &) {
    return {false, "INVALID_MARKET_DEFINITIONS"};
  }
}

int MarketDemandSystem::choiceTemperatureBasisPoints() const noexcept {
  return choiceTemperatureBasisPoints_;
}

} // namespace hh::game
