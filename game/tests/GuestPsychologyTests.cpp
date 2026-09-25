#include "hh/game/GuestGoals.h"
#include "hh/game/GuestPsychology.h"
#include <algorithm>
#include <bit>
#include <cmath>
#include <iostream>
#include <set>
#include <stdexcept>
#include <string>

using namespace hh::game;

namespace {
void require(bool value, const char *message) {
  if (!value)
    throw std::runtime_error(message);
}

template <typename Function>
void runCase(const char *name, Function function) {
  try {
    function();
  } catch (const std::exception &error) {
    throw std::runtime_error(std::string(name) + ": " + error.what());
  }
}

bool conflicts(std::uint32_t flags, GuestTrait a, GuestTrait b) {
  return (flags & guestTraitFlag(a)) != 0 &&
         (flags & guestTraitFlag(b)) != 0;
}

const PersonView &firstGuest(const SimulationView &view) {
  for (const auto &person : view.people)
    if (person.kind == PersonKind::Guest)
      return person;
  throw std::runtime_error("guest psychology fixture produced no guest");
}

void same_seed_and_guest_id_produce_identical_profile() {
  GuestPsychology a(42), b(42);
  const auto pa = a.generateGuestProfile(1001);
  const auto pb = b.generateGuestProfile(1001);
  require(pa == pb, "same seed and guest id produced different psychology");
  require(GuestPsychology::validProfile(pa),
          "deterministic generated profile was outside valid bounds");
}

void all_twenty_archetypes_are_reachable_and_profiles_are_bounded() {
  GuestPsychology psychology(20260909);
  std::set<GuestArchetype> archetypes;
  for (GuestId id = 1; id <= 5000; ++id) {
    const auto profile = psychology.generateGuestProfile(id);
    archetypes.insert(profile.archetype);
    require(GuestPsychology::validProfile(profile),
            "generated guest profile violated bounds");
    require(std::popcount(profile.traitFlags) <= 3,
            "generated guest received more than three traits");
    require(!conflicts(profile.traitFlags, GuestTrait::Patient,
                       GuestTrait::Impatient) &&
                !conflicts(profile.traitFlags, GuestTrait::Neat,
                           GuestTrait::Messy) &&
                !conflicts(profile.traitFlags, GuestTrait::LightSleeper,
                           GuestTrait::HeavySleeper) &&
                !conflicts(profile.traitFlags, GuestTrait::Social,
                           GuestTrait::Private) &&
                !conflicts(profile.traitFlags, GuestTrait::EarlyRiser,
                           GuestTrait::NightOwl),
            "generated guest received conflicting traits");

    switch (profile.archetype) {
    case GuestArchetype::DigitalNomad:
    case GuestArchetype::BleisureTraveler:
      require((profile.traitFlags & guestTraitFlag(GuestTrait::Workaholic)) != 0,
              "work-oriented archetype lost its signature Workaholic trait");
      break;
    case GuestArchetype::ExtendedStayGuest:
      require((profile.traitFlags & guestTraitFlag(GuestTrait::Private)) != 0,
              "extended-stay archetype lost its signature Private trait");
      break;
    case GuestArchetype::AirlineCrew:
      require((profile.traitFlags & guestTraitFlag(GuestTrait::LightSleeper)) != 0,
              "airline-crew archetype lost its signature LightSleeper trait");
      break;
    case GuestArchetype::WeddingGuest:
      require((profile.traitFlags & guestTraitFlag(GuestTrait::Social)) != 0,
              "wedding archetype lost its signature Social trait");
      break;
    case GuestArchetype::StaycationGuest:
      require((profile.traitFlags &
               guestTraitFlag(GuestTrait::StatusConscious)) != 0,
              "staycation archetype lost its signature StatusConscious trait");
      break;
    case GuestArchetype::SportsTeamTraveler:
      require((profile.traitFlags &
               guestTraitFlag(GuestTrait::FitnessFocused)) != 0,
              "sports-team archetype lost its signature FitnessFocused trait");
      break;
    default:
      break;
    }
  }
  require(archetypes.size() == 20,
          "not all twenty guest archetypes were reachable");
}

void profile_generation_is_keyed_by_stable_guest_identity() {
  GuestPsychology psychology(77);
  const auto first = psychology.generateGuestProfile(9001);
  for (int i = 0; i < 100; ++i)
    (void)psychology.generateGuestProfile(static_cast<GuestId>(10000 + i));
  const auto again = psychology.generateGuestProfile(9001);
  require(first == again,
          "guest profile changed after unrelated profile generation");
}

void business_preferences_follow_documented_archetype_priors() {
  GuestPsychology psychology(7801);
  GuestProfileView profile;
  profile.archetype = GuestArchetype::BusinessTraveler;
  psychology.initializeGuest(3001, profile);
  const auto state = psychology.snapshot(3001);
  require(state.has_value(), "business preference state was not initialized");
  require(state->preferences.wifi >= 8500 && state->preferences.desk >= 7000 &&
              state->preferences.breakfast >= 6000,
          "business guest missed documented high work/breakfast preferences");
  require(state->preferences.spa <= 2500 && state->preferences.pool <= 3000,
          "business guest missed documented low leisure preferences");
}

void new_archetypes_have_materially_distinct_hospitality_priorities() {
  const auto preferencesForArchetype = [](GuestArchetype archetype) {
    GuestPsychology psychology(7810 + static_cast<std::uint64_t>(archetype));
    GuestProfileView profile;
    profile.archetype = archetype;
    psychology.initializeGuest(4001, profile);
    return psychology.snapshot(4001)->preferences;
  };

  const auto nomad = preferencesForArchetype(GuestArchetype::DigitalNomad);
  require(nomad.wifi >= 9500 && nomad.desk >= 8500 && nomad.pool <= 3000,
          "digital nomad priorities are not work/connectivity differentiated");

  const auto bleisure =
      preferencesForArchetype(GuestArchetype::BleisureTraveler);
  require(bleisure.desk >= 8000 && bleisure.spa >= 5000 &&
              bleisure.roomQuality >= 8000,
          "bleisure priorities do not combine work and leisure demand");

  const auto extended =
      preferencesForArchetype(GuestArchetype::ExtendedStayGuest);
  require(extended.quietRoom >= 8500 && extended.desk >= 7000 &&
              extended.roomQuality >= 7500,
          "extended-stay priorities do not favor livability and quiet");

  const auto crew = preferencesForArchetype(GuestArchetype::AirlineCrew);
  require(crew.quietRoom >= 9500 && crew.roomQuality >= 8000 &&
              crew.social <= 4000,
          "airline-crew priorities do not strongly favor recovery and quiet");

  const auto wedding =
      preferencesForArchetype(GuestArchetype::WeddingGuest);
  require(wedding.social >= 9500 && wedding.breakfast >= 8000 &&
              wedding.desk <= 2000,
          "wedding guest priorities do not favor social/event behavior");

  const auto staycation =
      preferencesForArchetype(GuestArchetype::StaycationGuest);
  require(staycation.spa >= 8000 && staycation.pool >= 8500 &&
              staycation.roomQuality >= 8500,
          "staycation priorities do not favor hotel amenities and room quality");

  const auto team =
      preferencesForArchetype(GuestArchetype::SportsTeamTraveler);
  require(team.fitness >= 9500 && team.breakfast >= 9000 &&
              team.social >= 8500,
          "sports-team priorities do not favor fitness, food, and group activity");
}

void archetype_queue_tolerance_matches_trip_context() {
  GuestProfileView crew;
  crew.archetype = GuestArchetype::AirlineCrew;
  crew.patience = .5;

  GuestProfileView extended = crew;
  extended.archetype = GuestArchetype::ExtendedStayGuest;

  GuestProfileView team = crew;
  team.archetype = GuestArchetype::SportsTeamTraveler;

  const int crewTolerance = detail::queueToleranceFor(crew);
  const int extendedTolerance = detail::queueToleranceFor(extended);
  const int teamTolerance = detail::queueToleranceFor(team);
  require(crewTolerance < extendedTolerance,
          "airline crew should tolerate materially less queueing than extended stays");
  require(crewTolerance < teamTolerance,
          "airline crew should tolerate materially less queueing than sports groups");
}

void contextual_archetype_mix_responds_to_weekday_and_weekend_demand() {
  const auto sample = [](const GuestArchetypeContext &context,
                         std::uint64_t seed) {
    std::array<int, 20> counts{};
    std::mt19937_64 random(seed);
    for (int i = 0; i < 6000; ++i) {
      const auto profile =
          detail::generateGuestProfileFromRandom(random, context);
      ++counts[static_cast<std::size_t>(profile.archetype)];
    }
    return counts;
  };
  const auto count = [](const std::array<int, 20> &counts,
                        std::initializer_list<GuestArchetype> archetypes) {
    int total{};
    for (const auto archetype : archetypes)
      total += counts[static_cast<std::size_t>(archetype)];
    return total;
  };

  GuestArchetypeContext weekday;
  weekday.weekday = 1;
  weekday.hotelStars = 3;
  weekday.hotelReputation = 75;
  weekday.roomRateCents = 17000;
  auto weekend = weekday;
  weekend.weekday = 5;

  const auto weekdayCounts = sample(weekday, 88001);
  const auto weekendCounts = sample(weekend, 88001);
  const int workWeekday =
      count(weekdayCounts, {GuestArchetype::BusinessTraveler,
                            GuestArchetype::ExecutiveBusiness,
                            GuestArchetype::ConferenceDelegate,
                            GuestArchetype::DigitalNomad,
                            GuestArchetype::BleisureTraveler});
  const int workWeekend =
      count(weekendCounts, {GuestArchetype::BusinessTraveler,
                            GuestArchetype::ExecutiveBusiness,
                            GuestArchetype::ConferenceDelegate,
                            GuestArchetype::DigitalNomad,
                            GuestArchetype::BleisureTraveler});
  const int leisureWeekday =
      count(weekdayCounts, {GuestArchetype::CoupleLeisure,
                            GuestArchetype::FamilyLeisure,
                            GuestArchetype::LuxuryLeisure,
                            GuestArchetype::WellnessTraveler,
                            GuestArchetype::WeddingGuest,
                            GuestArchetype::StaycationGuest});
  const int leisureWeekend =
      count(weekendCounts, {GuestArchetype::CoupleLeisure,
                            GuestArchetype::FamilyLeisure,
                            GuestArchetype::LuxuryLeisure,
                            GuestArchetype::WellnessTraveler,
                            GuestArchetype::WeddingGuest,
                            GuestArchetype::StaycationGuest});
  require(workWeekday > workWeekend,
          "weekday context did not increase work-oriented guest mix");
  require(leisureWeekend > leisureWeekday,
          "weekend context did not increase leisure-oriented guest mix");
}

void contextual_archetype_mix_responds_to_property_positioning_and_events() {
  const auto sample = [](const GuestArchetypeContext &context,
                         std::uint64_t seed) {
    std::array<int, 20> counts{};
    std::mt19937_64 random(seed);
    for (int i = 0; i < 6000; ++i) {
      const auto profile =
          detail::generateGuestProfileFromRandom(random, context);
      ++counts[static_cast<std::size_t>(profile.archetype)];
    }
    return counts;
  };
  const auto count = [](const std::array<int, 20> &counts,
                        std::initializer_list<GuestArchetype> archetypes) {
    int total{};
    for (const auto archetype : archetypes)
      total += counts[static_cast<std::size_t>(archetype)];
    return total;
  };

  GuestArchetypeContext economyHotel;
  economyHotel.weekday = 5;
  economyHotel.hotelStars = 2;
  economyHotel.hotelReputation = 58;
  economyHotel.roomRateCents = 17000;

  GuestArchetypeContext resort = economyHotel;
  resort.hotelStars = 5;
  resort.hotelReputation = 92;

  const auto economyCounts = sample(economyHotel, 88002);
  const auto resortCounts = sample(resort, 88002);
  const int economyPremium =
      count(economyCounts, {GuestArchetype::LuxuryLeisure,
                            GuestArchetype::VipCelebrity,
                            GuestArchetype::WellnessTraveler,
                            GuestArchetype::StaycationGuest,
                            GuestArchetype::ExecutiveBusiness});
  const int resortPremium =
      count(resortCounts, {GuestArchetype::LuxuryLeisure,
                           GuestArchetype::VipCelebrity,
                           GuestArchetype::WellnessTraveler,
                           GuestArchetype::StaycationGuest,
                           GuestArchetype::ExecutiveBusiness});
  require(resortPremium > economyPremium,
          "premium amenity-rich property did not attract more premium archetypes");

  GuestArchetypeContext quietWeekday;
  quietWeekday.weekday = 2;
  quietWeekday.hotelStars = 3;
  quietWeekday.hotelReputation = 75;
  quietWeekday.roomRateCents = 17000;
  auto eventDay = quietWeekday;
  eventDay.eventAttendees = 300;

  const auto quietCounts = sample(quietWeekday, 88003);
  const auto eventCounts = sample(eventDay, 88003);
  const int quietEventGuests =
      count(quietCounts, {GuestArchetype::ConferenceDelegate,
                          GuestArchetype::WeddingGuest,
                          GuestArchetype::GroupTourTraveler,
                          GuestArchetype::BleisureTraveler});
  const int eventGuests =
      count(eventCounts, {GuestArchetype::ConferenceDelegate,
                          GuestArchetype::WeddingGuest,
                          GuestArchetype::GroupTourTraveler,
                          GuestArchetype::BleisureTraveler});
  require(eventGuests > quietEventGuests,
          "on-property event demand did not increase event-linked archetypes");
}

void contextual_archetype_mix_responds_to_pricing_and_amenities() {
  const auto sample = [](const GuestArchetypeContext &context,
                         std::uint64_t seed) {
    std::array<int, 20> counts{};
    std::mt19937_64 random(seed);
    for (int i = 0; i < 8000; ++i) {
      const auto profile =
          detail::generateGuestProfileFromRandom(random, context);
      ++counts[static_cast<std::size_t>(profile.archetype)];
    }
    return counts;
  };
  const auto count = [](const std::array<int, 20> &counts,
                        std::initializer_list<GuestArchetype> archetypes) {
    int total{};
    for (const auto archetype : archetypes)
      total += counts[static_cast<std::size_t>(archetype)];
    return total;
  };

  GuestArchetypeContext valueRate;
  valueRate.weekday = 2;
  valueRate.hotelStars = 3;
  valueRate.hotelReputation = 75;
  valueRate.roomRateCents = 9000;
  auto premiumRate = valueRate;
  premiumRate.roomRateCents = 28000;

  const auto valueRateCounts = sample(valueRate, 88006);
  const auto premiumRateCounts = sample(premiumRate, 88006);
  const int valueSegmentsAtValueRate =
      count(valueRateCounts, {GuestArchetype::BudgetLeisure,
                              GuestArchetype::Backpacker,
                              GuestArchetype::GroupTourTraveler,
                              GuestArchetype::ExtendedStayGuest,
                              GuestArchetype::SportsTeamTraveler});
  const int valueSegmentsAtPremiumRate =
      count(premiumRateCounts, {GuestArchetype::BudgetLeisure,
                                GuestArchetype::Backpacker,
                                GuestArchetype::GroupTourTraveler,
                                GuestArchetype::ExtendedStayGuest,
                                GuestArchetype::SportsTeamTraveler});
  const int premiumSegmentsAtValueRate =
      count(valueRateCounts, {GuestArchetype::ExecutiveBusiness,
                              GuestArchetype::LuxuryLeisure,
                              GuestArchetype::VipCelebrity,
                              GuestArchetype::BleisureTraveler,
                              GuestArchetype::CriticReviewer,
                              GuestArchetype::WellnessTraveler});
  const int premiumSegmentsAtPremiumRate =
      count(premiumRateCounts, {GuestArchetype::ExecutiveBusiness,
                                GuestArchetype::LuxuryLeisure,
                                GuestArchetype::VipCelebrity,
                                GuestArchetype::BleisureTraveler,
                                GuestArchetype::CriticReviewer,
                                GuestArchetype::WellnessTraveler});
  require(valueSegmentsAtValueRate > valueSegmentsAtPremiumRate,
          "premium pricing did not reduce price-sensitive guest share");
  require(premiumSegmentsAtPremiumRate > premiumSegmentsAtValueRate,
          "premium pricing did not increase higher-budget guest share");

  GuestArchetypeContext weakAmenities;
  weakAmenities.weekday = 5;
  weakAmenities.hotelStars = 3;
  weakAmenities.hotelReputation = 75;
  weakAmenities.roomRateCents = 17000;
  weakAmenities.gymScore = 25;
  weakAmenities.spaScore = 25;
  weakAmenities.poolScore = 25;
  auto strongAmenities = weakAmenities;
  strongAmenities.gymScore = 95;
  strongAmenities.spaScore = 95;
  strongAmenities.poolScore = 95;

  const auto weakAmenityCounts = sample(weakAmenities, 88007);
  const auto strongAmenityCounts = sample(strongAmenities, 88007);
  const int matchedWithWeakAmenities =
      count(weakAmenityCounts, {GuestArchetype::WellnessTraveler,
                                GuestArchetype::StaycationGuest,
                                GuestArchetype::FamilyLeisure,
                                GuestArchetype::SportsTeamTraveler,
                                GuestArchetype::LuxuryLeisure,
                                GuestArchetype::CoupleLeisure});
  const int matchedWithStrongAmenities =
      count(strongAmenityCounts, {GuestArchetype::WellnessTraveler,
                                  GuestArchetype::StaycationGuest,
                                  GuestArchetype::FamilyLeisure,
                                  GuestArchetype::SportsTeamTraveler,
                                  GuestArchetype::LuxuryLeisure,
                                  GuestArchetype::CoupleLeisure});
  require(matchedWithStrongAmenities > matchedWithWeakAmenities,
          "stronger amenities did not increase amenity-matched guest share");
}

void contextual_archetype_mix_responds_to_seasonality_and_location() {
  const auto sample = [](const GuestArchetypeContext &context,
                         std::uint64_t seed) {
    std::array<int, 20> counts{};
    std::mt19937_64 random(seed);
    for (int i = 0; i < 8000; ++i) {
      const auto profile =
          detail::generateGuestProfileFromRandom(random, context);
      ++counts[static_cast<std::size_t>(profile.archetype)];
    }
    return counts;
  };
  const auto count = [](const std::array<int, 20> &counts,
                        std::initializer_list<GuestArchetype> archetypes) {
    int total{};
    for (const auto archetype : archetypes)
      total += counts[static_cast<std::size_t>(archetype)];
    return total;
  };

  GuestArchetypeContext trough;
  trough.weekday = 2;
  trough.hotelStars = 3;
  trough.hotelReputation = 75;
  trough.roomRateCents = 17000;
  trough.seasonMultiplierBasisPoints = 6500;
  auto peak = trough;
  peak.seasonMultiplierBasisPoints = 17500;

  const auto troughCounts = sample(trough, 88004);
  const auto peakCounts = sample(peak, 88004);
  const int troughLeisure =
      count(troughCounts, {GuestArchetype::CoupleLeisure,
                           GuestArchetype::FamilyLeisure,
                           GuestArchetype::LuxuryLeisure,
                           GuestArchetype::WellnessTraveler,
                           GuestArchetype::WeddingGuest,
                           GuestArchetype::StaycationGuest});
  const int peakLeisure =
      count(peakCounts, {GuestArchetype::CoupleLeisure,
                         GuestArchetype::FamilyLeisure,
                         GuestArchetype::LuxuryLeisure,
                         GuestArchetype::WellnessTraveler,
                         GuestArchetype::WeddingGuest,
                         GuestArchetype::StaycationGuest});
  const int troughRecurring =
      count(troughCounts, {GuestArchetype::BusinessTraveler,
                           GuestArchetype::ExecutiveBusiness,
                           GuestArchetype::ExtendedStayGuest,
                           GuestArchetype::AirlineCrew});
  const int peakRecurring =
      count(peakCounts, {GuestArchetype::BusinessTraveler,
                         GuestArchetype::ExecutiveBusiness,
                         GuestArchetype::ExtendedStayGuest,
                         GuestArchetype::AirlineCrew});
  require(peakLeisure > troughLeisure,
          "peak season did not increase discretionary leisure mix");
  require(troughRecurring > peakRecurring,
          "low season did not increase recurring work/long-stay share");

  GuestArchetypeContext weakLocation;
  weakLocation.weekday = 2;
  weakLocation.hotelStars = 4;
  weakLocation.hotelReputation = 82;
  weakLocation.roomRateCents = 19000;
  weakLocation.locationScore = 30;
  auto strongLocation = weakLocation;
  strongLocation.locationScore = 90;

  const auto weakCounts = sample(weakLocation, 88005);
  const auto strongCounts = sample(strongLocation, 88005);
  const int weakLocationSensitive =
      count(weakCounts, {GuestArchetype::BusinessTraveler,
                         GuestArchetype::ExecutiveBusiness,
                         GuestArchetype::LuxuryLeisure,
                         GuestArchetype::ConferenceDelegate,
                         GuestArchetype::WellnessTraveler,
                         GuestArchetype::BleisureTraveler});
  const int strongLocationSensitive =
      count(strongCounts, {GuestArchetype::BusinessTraveler,
                           GuestArchetype::ExecutiveBusiness,
                           GuestArchetype::LuxuryLeisure,
                           GuestArchetype::ConferenceDelegate,
                           GuestArchetype::WellnessTraveler,
                           GuestArchetype::BleisureTraveler});
  const int weakValueLongStay =
      count(weakCounts, {GuestArchetype::BudgetLeisure,
                         GuestArchetype::Backpacker,
                         GuestArchetype::ExtendedStayGuest,
                         GuestArchetype::GroupTourTraveler});
  const int strongValueLongStay =
      count(strongCounts, {GuestArchetype::BudgetLeisure,
                           GuestArchetype::Backpacker,
                           GuestArchetype::ExtendedStayGuest,
                           GuestArchetype::GroupTourTraveler});
  require(strongLocationSensitive > weakLocationSensitive,
          "strong location did not increase location-sensitive archetypes");
  require(weakValueLongStay > strongValueLongStay,
          "weak location did not increase value/long-stay share");
}

void archetypes_generate_trip_length_profiles() {
  GuestProfileView profile;

  profile.archetype = GuestArchetype::AirlineCrew;
  require(detail::stayNightsFor(profile, 99) == 1,
          "airline crew should be a one-night archetype");

  profile.archetype = GuestArchetype::AirportTransitTraveler;
  require(detail::stayNightsFor(profile, 99) == 1,
          "airport transit should be a one-night archetype");

  profile.archetype = GuestArchetype::ExtendedStayGuest;
  const int extended = detail::stayNightsFor(profile, 14);
  require(extended >= 7 && extended <= 21,
          "extended-stay archetype left its 7-21 night range");

  profile.archetype = GuestArchetype::DigitalNomad;
  const int nomad = detail::stayNightsFor(profile, 7);
  require(nomad >= 3 && nomad <= 10,
          "digital nomad archetype left its 3-10 night range");

  profile.archetype = GuestArchetype::FamilyLeisure;
  const int family = detail::stayNightsFor(profile, 8);
  require(family >= 3 && family <= 5,
          "family leisure archetype left its 3-5 night range");

  profile.archetype = GuestArchetype::StaycationGuest;
  const int staycation = detail::stayNightsFor(profile, 3);
  require(staycation >= 1 && staycation <= 2,
          "staycation archetype left its 1-2 night range");
}

void traits_materially_modify_guest_preferences() {
  constexpr GuestId id = 3002;
  GuestProfileView baselineProfile;
  baselineProfile.archetype = GuestArchetype::BusinessTraveler;

  GuestPsychology baseline(7802);
  baseline.initializeGuest(id, baselineProfile);
  const auto before = *baseline.snapshot(id);

  auto traitProfile = baselineProfile;
  traitProfile.traitFlags = guestTraitFlag(GuestTrait::Workaholic) |
                            guestTraitFlag(GuestTrait::Foodie) |
                            guestTraitFlag(GuestTrait::FitnessFocused);
  GuestPsychology modified(7802);
  modified.initializeGuest(id, traitProfile);
  const auto after = *modified.snapshot(id);

  require(after.preferences.desk > before.preferences.desk &&
              after.preferences.wifi > before.preferences.wifi,
          "Workaholic trait did not numerically raise work preferences");
  require(after.preferences.breakfast > before.preferences.breakfast,
          "Foodie trait did not numerically raise food preference");
  require(after.preferences.fitness > before.preferences.fitness,
          "FitnessFocused trait did not numerically raise fitness preference");
}

void awake_and_sleeping_need_updates_use_hmg_rates() {
  GuestPsychology psychology(88);
  const GuestId id = 2001;
  psychology.initializeGuest(id, psychology.generateGuestProfile(id));
  const auto initial = psychology.snapshot(id);
  require(initial.has_value(), "guest psychology state was not initialized");

  psychology.updateNeeds(id, 3600, false);
  auto awake = psychology.snapshot(id);
  require(awake.has_value() &&
              awake->needs.hunger == initial->needs.hunger - 10 &&
              awake->needs.energy == initial->needs.energy - 5,
          "awake need decay did not use HMG-010 hourly rates");

  psychology.updateNeeds(id, 3600, true);
  auto sleeping = psychology.snapshot(id);
  require(sleeping.has_value() &&
              sleeping->needs.energy ==
                  std::min(100, awake->needs.energy + 22) &&
              sleeping->needs.hunger == awake->needs.hunger,
          "sleeping need recovery did not use HMG-010 energy rate");
}

void negative_service_experience_creates_attributed_memory_and_complaint() {
  GuestPsychology psychology(89);
  const GuestId id = 2002;
  auto profile = psychology.generateGuestProfile(id);
  profile.serviceSensitivity = 1.0;
  psychology.initializeGuest(id, profile);

  ExperienceEvent event;
  event.type = ExperienceEventType::LongCheckInQueue;
  event.timestampSeconds = 3600;
  event.category = ExperienceCategory::ArrivalDeparture;
  event.observedValue = 45 * 60;
  event.expectedValue = 8 * 60;
  event.rawImpact = -30;
  event.memorySalience = 10000;
  event.memoryHalfLifeHours = 24;
  event.complaintEligible = true;
  psychology.recordExperience(id, event);

  const auto state = psychology.snapshot(id);
  require(state.has_value(), "guest psychology state disappeared");
  require(state->satisfaction.checkIn < 50 &&
              state->satisfaction.arrivalDeparture < 50,
          "long check-in wait was not attributed to arrival satisfaction");
  require(state->memories.size() == 1 &&
              state->memories.front().type ==
                  ExperienceEventType::LongCheckInQueue &&
              state->memories.front().magnitude == 30 &&
              !state->memories.front().resolved,
          "negative service event did not create an attributable memory");
  require(state->complaints.size() == 1 &&
              state->complaints.front().type == ComplaintType::CheckInDelay &&
              state->complaints.front().urgency == ComplaintUrgency::Low,
          "complaint-eligible check-in delay did not create the required complaint");
}

void memory_contribution_halves_at_configured_half_life() {
  GuestMemory memory;
  memory.valence = -1;
  memory.magnitude = 60;
  memory.salience = 10000;
  memory.timestampSeconds = 10 * 3600;
  memory.halfLifeHours = 12;
  const double now = GuestPsychology::memoryContribution(memory, 10 * 3600);
  const double later = GuestPsychology::memoryContribution(memory, 22 * 3600);
  require(std::abs(later - now * .5) < 1e-9,
          "guest memory did not halve after one configured half-life");
}

void complaint_threshold_respects_magnitude_and_guest_sensitivity() {
  GuestPsychology psychology(90);
  const GuestId id = 2003;
  auto profile = psychology.generateGuestProfile(id);
  profile.serviceSensitivity = .20;
  profile.traitFlags &= ~guestTraitFlag(GuestTrait::ComplaintProne);
  psychology.initializeGuest(id, profile);

  ExperienceEvent event;
  event.type = ExperienceEventType::LongCheckInQueue;
  event.timestampSeconds = 100;
  event.category = ExperienceCategory::ArrivalDeparture;
  event.rawImpact = -30;
  event.memorySalience = 10000;
  event.memoryHalfLifeHours = 24;
  event.complaintEligible = true;
  psychology.recordExperience(id, event);
  require(psychology.snapshot(id)->complaints.empty(),
          "low-sensitivity guest complained below HMG-010 magnitude override");

  event.timestampSeconds = 200;
  event.rawImpact = -50;
  psychology.recordExperience(id, event);
  require(psychology.snapshot(id)->complaints.size() == 1,
          "magnitude-50 complaint override was not honored");
}

void live_simulation_exposes_authoritative_psychology_and_updates_needs() {
  auto sim = Simulation::tutorial(169);
  require(sim.loadDefinitions(R"({"baseDemand":100})").ok,
          "live psychology fixture definitions rejected");
  sim.step(3600);
  const auto guest = firstGuest(sim.view());
  const auto before = sim.guestPsychology(guest.reservationId);
  require(before.guestId == guest.reservationId && before.profile == guest.profile,
          "live guest did not expose matching authoritative psychology");

  sim.step(3600);
  const auto after = sim.guestPsychology(guest.reservationId);
  require(after.needs.hunger <= before.needs.hunger - 9 &&
              after.needs.energy <= before.needs.energy - 4,
          "live one-second simulation did not advance authoritative guest needs");
}

void severe_live_check_in_wait_creates_retained_memory_and_complaint() {
  auto sim = Simulation::tutorial(169);
  require(sim.loadDefinitions(R"({"baseDemand":100})").ok,
          "wait psychology fixture definitions rejected");
  EntityId receptionist{};
  for (const auto &person : sim.view().people)
    if (person.kind == PersonKind::Receptionist) {
      receptionist = person.id;
      break;
    }
  require(receptionist && sim.fireStaff(receptionist).ok,
          "wait psychology fixture could not remove receptionist");
  sim.step(3600);
  const auto guestId = firstGuest(sim.view()).reservationId;
  sim.step(1900);
  const auto psychology = sim.guestPsychology(guestId);
  const bool hasWaitMemory = std::any_of(
      psychology.memories.begin(), psychology.memories.end(),
      [](const GuestMemory &memory) {
        return memory.type == ExperienceEventType::LongCheckInQueue;
      });
  const bool hasDelayComplaint = std::any_of(
      psychology.complaints.begin(), psychology.complaints.end(),
      [](const Complaint &complaint) {
        return complaint.type == ComplaintType::CheckInDelay;
      });
  require(hasWaitMemory && hasDelayComplaint,
          "severe live check-in wait lacked attributed memory/complaint");
}

void simulation_exposes_deterministic_goal_selection_interface() {
  auto sim = Simulation::tutorial(171);
  GuestOpportunitySnapshot opportunities;
  GoalOpportunity relax;
  relax.stableGoalId = 20;
  relax.goal = GuestGoalClass::Relax;
  relax.needScore = 30;
  GoalOpportunity eat = relax;
  eat.stableGoalId = 10;
  eat.goal = GuestGoalClass::Eat;
  opportunities.opportunities = {relax, eat};
  const auto selected = sim.chooseGuestGoal(55, opportunities);
  require(selected.valid && selected.stableGoalId == 10,
          "Simulation guest-goal interface did not preserve stable tie-break");
}

void live_guest_psychology_survives_save_round_trip() {
  auto sim = Simulation::tutorial(172);
  require(sim.loadDefinitions(R"({"baseDemand":100})").ok,
          "psychology save fixture definitions rejected");
  sim.step(3600);
  const auto guestId = firstGuest(sim.view()).reservationId;
  sim.step(733);
  const auto before = sim.guestPsychology(guestId);
  const auto loaded = Simulation::load(sim.save());
  const auto after = loaded.guestPsychology(guestId);
  require(before == after,
          "authoritative guest psychology did not survive save/load exactly");
  require(loaded.save() == sim.save(),
          "psychology save was not byte-stable after round trip");
}

void repeat_intent_responds_to_expectation_adjusted_satisfaction() {
  GuestPsychologySnapshot high;
  high.satisfaction.overall = 92;
  high.satisfaction.expectationFit = 90;
  high.operational.valuePerception = 85;
  high.expectations.room = high.expectations.cleanliness =
      high.expectations.service = high.expectations.food =
          high.expectations.amenities = high.expectations.quiet =
              high.expectations.convenience = high.expectations.value = 80;
  auto low = high;
  low.satisfaction.overall = 55;
  low.satisfaction.expectationFit = 45;
  low.operational.valuePerception = 50;
  require(calculateRepeatIntent(high, 0) > calculateRepeatIntent(low, 0),
          "repeat intent ignored expectation-adjusted satisfaction");
}

void material_memories_influence_repeat_intent() {
  GuestPsychologySnapshot positive;
  positive.satisfaction.overall = 80;
  positive.satisfaction.expectationFit = 80;
  positive.operational.valuePerception = 80;
  GuestMemory good;
  good.valence = 1;
  good.magnitude = 50;
  good.salience = 10000;
  good.halfLifeHours = 48;
  positive.memories.push_back(good);
  auto negative = positive;
  negative.memories.front().valence = -1;
  require(calculateRepeatIntent(positive, 0) >
              calculateRepeatIntent(negative, 0),
          "repeat intent ignored material memory sentiment");
}

void material_experience_persists_in_versioned_save() {
  auto sim = Simulation::tutorial(174);
  require(sim.loadDefinitions(R"({"baseDemand":100})").ok,
          "material experience fixture definitions rejected");
  sim.step(3600);
  const auto guestId = firstGuest(sim.view()).reservationId;
  ExperienceEvent meal;
  meal.type = ExperienceEventType::GreatMeal;
  meal.timestampSeconds = sim.view().elapsedSeconds;
  meal.category = ExperienceCategory::Food;
  meal.observedValue = 95;
  meal.expectedValue = 75;
  meal.rawImpact = 25;
  meal.memorySalience = 10000;
  meal.memoryHalfLifeHours = 48;
  require(sim.recordGuestExperience(guestId, meal).ok,
          "simulation rejected material guest experience");
  const auto before = sim.guestPsychology(guestId);
  require(std::any_of(before.memories.begin(), before.memories.end(),
                      [](const GuestMemory &memory) {
                        return memory.type == ExperienceEventType::GreatMeal;
                      }),
          "material guest experience was not retained before save");
  const auto saved = sim.save();
  require(saved.rfind("HHGS 12 ", 0) == 0,
          "psychology schema change did not advance save version to 12");
  const auto loaded = Simulation::load(saved);
  require(loaded.guestPsychology(guestId) == before,
          "material psychology history changed across v12 save/load");
  require(loaded.save() == saved,
          "v12 psychology save was not byte-stable after round trip");
}
} // namespace

int main() {
  try {
    runCase("same_seed_and_guest_id_produce_identical_profile", same_seed_and_guest_id_produce_identical_profile);
    runCase("all_twenty_archetypes_are_reachable_and_profiles_are_bounded", all_twenty_archetypes_are_reachable_and_profiles_are_bounded);
    runCase("profile_generation_is_keyed_by_stable_guest_identity", profile_generation_is_keyed_by_stable_guest_identity);
    runCase("business_preferences_follow_documented_archetype_priors", business_preferences_follow_documented_archetype_priors);
    runCase("new_archetypes_have_materially_distinct_hospitality_priorities", new_archetypes_have_materially_distinct_hospitality_priorities);
    runCase("archetype_queue_tolerance_matches_trip_context", archetype_queue_tolerance_matches_trip_context);
    runCase("contextual_archetype_mix_responds_to_weekday_and_weekend_demand", contextual_archetype_mix_responds_to_weekday_and_weekend_demand);
    runCase("contextual_archetype_mix_responds_to_property_positioning_and_events", contextual_archetype_mix_responds_to_property_positioning_and_events);
    runCase("contextual_archetype_mix_responds_to_pricing_and_amenities", contextual_archetype_mix_responds_to_pricing_and_amenities);
    runCase("contextual_archetype_mix_responds_to_seasonality_and_location", contextual_archetype_mix_responds_to_seasonality_and_location);
    runCase("archetypes_generate_trip_length_profiles", archetypes_generate_trip_length_profiles);
    runCase("traits_materially_modify_guest_preferences", traits_materially_modify_guest_preferences);
    runCase("awake_and_sleeping_need_updates_use_hmg_rates", awake_and_sleeping_need_updates_use_hmg_rates);
    runCase("negative_service_experience_creates_attributed_memory_and_complaint", negative_service_experience_creates_attributed_memory_and_complaint);
    runCase("memory_contribution_halves_at_configured_half_life", memory_contribution_halves_at_configured_half_life);
    runCase("complaint_threshold_respects_magnitude_and_guest_sensitivity", complaint_threshold_respects_magnitude_and_guest_sensitivity);
    runCase("live_simulation_exposes_authoritative_psychology_and_updates_needs", live_simulation_exposes_authoritative_psychology_and_updates_needs);
    runCase("severe_live_check_in_wait_creates_retained_memory_and_complaint", severe_live_check_in_wait_creates_retained_memory_and_complaint);
    runCase("simulation_exposes_deterministic_goal_selection_interface", simulation_exposes_deterministic_goal_selection_interface);
    runCase("live_guest_psychology_survives_save_round_trip", live_guest_psychology_survives_save_round_trip);
    runCase("repeat_intent_responds_to_expectation_adjusted_satisfaction", repeat_intent_responds_to_expectation_adjusted_satisfaction);
    runCase("material_memories_influence_repeat_intent", material_memories_influence_repeat_intent);
    runCase("material_experience_persists_in_versioned_save", material_experience_persists_in_versioned_save);
  } catch (const std::exception &error) {
    std::cerr << "FAIL: " << error.what() << '\n';
    return 1;
  }
  std::cout << "All guest psychology tests passed\n";
  return 0;
}
