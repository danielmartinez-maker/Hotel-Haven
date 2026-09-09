#include "hh/game/Simulation.h"
#include <algorithm>
#include <array>
#include <bit>
#include <climits>
#include <cmath>
#include <iomanip>
#include <iostream>
#include <set>
#include <sstream>
#include <stdexcept>
#include <string>

using namespace hh::game;
static void require(bool v, const char *m) {
  if (!v)
    throw std::runtime_error(m);
}
static std::string with_save_version(std::string save, int version) {
  const auto end = save.find(' ', 5);
  if (!save.starts_with("HHGS ") || end == std::string::npos)
    throw std::runtime_error("save fixture has no version token");
  save.replace(5, end - 5, std::to_string(version));
  return save;
}
static const RoomView &room(const SimulationView &v, EntityId id) {
  for (auto &r : v.rooms)
    if (r.id == id)
      return r;
  throw std::runtime_error("room missing");
}

static bool same_profile(const GuestProfileView &a,
                         const GuestProfileView &b) {
  return a.archetype == b.archetype &&
         a.budgetPerNightCents == b.budgetPerNightCents &&
         a.priceSensitivity == b.priceSensitivity &&
         a.serviceSensitivity == b.serviceSensitivity &&
         a.cleanlinessSensitivity == b.cleanlinessSensitivity &&
         a.noiseSensitivity == b.noiseSensitivity &&
         a.privacySensitivity == b.privacySensitivity &&
         a.safetySensitivity == b.safetySensitivity &&
         a.comfortSensitivity == b.comfortSensitivity &&
         a.foodSensitivity == b.foodSensitivity && a.patience == b.patience &&
         a.traitFlags == b.traitFlags;
}

static void guest_profiles_are_deterministic_and_bounded() {
  std::array<bool, 13> seen{};
  for (std::uint64_t seed = 40; seed < 168; ++seed) {
    auto a = Simulation::tutorial(seed);
    auto b = Simulation::tutorial(seed);
    require(a.loadDefinitions(R"({"baseDemand":100})").ok &&
                b.loadDefinitions(R"({"baseDemand":100})").ok,
            "profile test definitions rejected");
    for (const auto &room : a.view().rooms)
      require(a.setRoomRate(room.id, 50).ok,
              "profile test could not set inclusive room rate");
    for (const auto &room : b.view().rooms)
      require(b.setRoomRate(room.id, 50).ok,
              "profile test could not set matching room rate");
    a.step(3600);
    b.step(3600);
    const auto av = a.view();
    const auto bv = b.view();
    require(av.reservations.size() == bv.reservations.size() &&
                !av.reservations.empty(),
            "profile test did not create matching demand");
    for (std::size_t index = 0; index < av.reservations.size(); ++index) {
      const auto &profile = av.reservations[index].profile;
      require(same_profile(profile, bv.reservations[index].profile),
              "same seed generated a different guest profile");
      const auto archetype = static_cast<int>(profile.archetype);
      require(archetype >= 0 && archetype < static_cast<int>(seen.size()),
              "guest archetype outside the public range");
      seen[static_cast<std::size_t>(archetype)] = true;
      require(profile.budgetPerNightCents >= 4000 &&
                  profile.budgetPerNightCents <= 1'000'000,
              "guest budget outside the supported range");
      const std::array<double, 9> factors = {
          profile.priceSensitivity,       profile.serviceSensitivity,
          profile.cleanlinessSensitivity, profile.noiseSensitivity,
          profile.privacySensitivity,     profile.safetySensitivity,
          profile.comfortSensitivity,     profile.foodSensitivity,
          profile.patience};
      for (double factor : factors)
        require(std::isfinite(factor) && factor >= 0 && factor <= 1,
                "guest factor outside normalized range");
      require(std::popcount(profile.traitFlags) <= 3,
              "guest received more than three traits");
      const auto has = [&](GuestTrait trait) {
        return (profile.traitFlags & guestTraitFlag(trait)) != 0;
      };
      require(!(has(GuestTrait::Patient) && has(GuestTrait::Impatient)) &&
                  !(has(GuestTrait::Neat) && has(GuestTrait::Messy)) &&
                  !(has(GuestTrait::LightSleeper) &&
                    has(GuestTrait::HeavySleeper)) &&
                  !(has(GuestTrait::Social) && has(GuestTrait::Private)) &&
                  !(has(GuestTrait::EarlyRiser) &&
                    has(GuestTrait::NightOwl)),
              "guest received contradictory traits");
    }
  }
  require(std::all_of(seen.begin(), seen.end(), [](bool value) { return value; }),
          "deterministic market did not expose every guest archetype");
  require(static_cast<int>(GuestArchetype::CriticReviewer) == 12,
          "public guest archetype catalog is incomplete");
}

static void guest_profiles_propagate_and_round_trip() {
  auto s = Simulation::tutorial(169);
  require(s.loadDefinitions(R"({"baseDemand":100})").ok,
          "profile propagation definitions rejected");
  s.step(3600);
  const auto before = s.view();
  const PersonView *guest = nullptr;
  const ReservationView *reservation = nullptr;
  for (const auto &person : before.people)
    if (person.kind == PersonKind::Guest) {
      guest = &person;
      break;
    }
  require(guest && guest->reservationId,
          "arriving guest lacks a public reservation reference");
  for (const auto &candidate : before.reservations)
    if (candidate.id == guest->reservationId)
      reservation = &candidate;
  require(reservation && same_profile(guest->profile, reservation->profile),
          "reservation profile did not propagate to the arriving guest");
  require(guest->queueToleranceSeconds >= 120 &&
              guest->queueToleranceSeconds <= 1200,
          "guest queue tolerance is not a useful personal threshold");

  const auto saved = s.save();
  require(saved.starts_with("HHGS 10 "),
          "check-in diagnostics did not bump the save format");
  const auto loaded = Simulation::load(saved).view();
  const PersonView *loadedGuest = nullptr;
  const ReservationView *loadedReservation = nullptr;
  for (const auto &person : loaded.people)
    if (person.id == guest->id)
      loadedGuest = &person;
  for (const auto &candidate : loaded.reservations)
    if (candidate.id == reservation->id)
      loadedReservation = &candidate;
  require(loadedGuest && loadedReservation &&
              loadedGuest->queueToleranceSeconds ==
                  guest->queueToleranceSeconds &&
              same_profile(loadedGuest->profile, guest->profile) &&
              same_profile(loadedReservation->profile, reservation->profile),
          "guest profile did not round-trip exactly");
  require(Simulation::load(saved).save() == saved,
          "v10 guest save is not byte-stable after loading");

  auto corrupted = saved;
  const auto guestMarker = '"' + guest->name + '"';
  const auto guestPosition = corrupted.find(guestMarker);
  const auto guestLineEnd = corrupted.find('\n', guestPosition);
  const auto validBudget = std::to_string(guest->profile.budgetPerNightCents);
  const auto budgetPosition = corrupted.find(' ' + validBudget + ' ',
                                             guestPosition);
  require(guestPosition != std::string::npos &&
              guestLineEnd != std::string::npos &&
              budgetPosition != std::string::npos &&
              budgetPosition < guestLineEnd,
          "serialized guest profile was not found on the guest record");
  corrupted.replace(budgetPosition + 1, validBudget.size(), "3999");
  bool rejected = false;
  try {
    (void)Simulation::load(corrupted);
  } catch (const std::invalid_argument &) {
    rejected = true;
  }
  require(rejected, "invalid serialized guest profile was accepted");

  auto legacy = Simulation(170, 4, 4, 1).save();
  require(legacy.starts_with("HHGS 10 "),
          "empty v10 migration fixture failed");
  for (const int version : {9, 8, 7})
    require(Simulation::load(with_save_version(legacy, version))
                .save()
                .starts_with("HHGS 10 "),
            "legacy save did not migrate to the v10 writer");
}

static void reviews_follow_segment_probability_rules() {
  bool foundCritic = false;
  bool foundOrdinaryReview = false;
  bool foundOrdinarySkip = false;
  for (std::uint64_t seed = 250;
       seed < 270 &&
       !(foundCritic && foundOrdinaryReview && foundOrdinarySkip);
       ++seed) {
    auto s = Simulation::tutorial(seed);
    require(
        s.loadDefinitions(
             R"({"baseDemand":100,"roomConditionLossPerDay":0,"initialLinen":500,"initialTowels":1000,"initialAmenities":500,"initialChemicals":500})")
            .ok,
        "review probability definitions rejected");
    s.step(20 * 86400);
    const auto view = s.view();
    std::set<EntityId> reviewed;
    for (const auto &review : view.reviews) {
      require(std::isfinite(review.score) && review.score >= 1.0 &&
                  review.score <= 10.0,
              "review score is outside the 1.0-10.0 scale");
      reviewed.insert(review.reservationId);
    }
    for (const auto &reservation : view.reservations) {
      if (!reservation.completed)
        continue;
      const bool hasReview = reviewed.contains(reservation.id);
      if (reservation.profile.archetype == GuestArchetype::CriticReviewer) {
        foundCritic = true;
        require(hasReview, "completed critic stay did not force a review");
      } else if (hasReview) {
        foundOrdinaryReview = true;
      } else {
        foundOrdinarySkip = true;
      }
    }
  }
  require(foundCritic, "review fixture did not complete a critic stay");
  require(foundOrdinaryReview && foundOrdinarySkip,
          "ordinary reviews were not generated probabilistically");
}

static void populated_v8_review_scores_migrate_to_ten_point_scale() {
  auto s = Simulation::tutorial(9);
  s.step(5 * 86400);
  const auto before = s.view();
  require(!before.reviews.empty(),
          "populated v8 migration fixture produced no reviews");
  auto legacy = s.save();
  for (const auto &reservation : before.reservations) {
    std::ostringstream profile;
    profile << std::setprecision(17)
            << static_cast<int>(reservation.profile.archetype) << ' '
            << reservation.profile.budgetPerNightCents << ' '
            << reservation.profile.priceSensitivity << ' '
            << reservation.profile.serviceSensitivity << ' '
            << reservation.profile.cleanlinessSensitivity << ' '
            << reservation.profile.noiseSensitivity << ' '
            << reservation.profile.privacySensitivity << ' '
            << reservation.profile.safetySensitivity << ' '
            << reservation.profile.comfortSensitivity << ' '
            << reservation.profile.foodSensitivity << ' '
            << reservation.profile.patience << ' '
            << reservation.profile.traitFlags;
    const auto oldSuffix = ' ' + profile.str() + ' ' +
                           std::to_string(reservation.walkedRelocated) + ' ' +
                           std::to_string(reservation.checkInTravelSeconds) +
                           ' ' +
                           std::to_string(reservation.checkInWaitSeconds) + '\n';
    const auto legacySuffix =
        ' ' + profile.str() + ' ' +
        std::to_string(reservation.walkedRelocated) + '\n';
    const auto position = legacy.find(oldSuffix);
    require(position != std::string::npos,
            "v10 reservation diagnostics were not found in the save");
    legacy.replace(position, oldSuffix.size(), legacySuffix);
  }
  const auto migratedV9 = Simulation::load(with_save_version(legacy, 9));
  for (const auto &reservation : migratedV9.view().reservations)
    require(reservation.checkInTravelSeconds == 0 &&
                reservation.checkInWaitSeconds == 0,
            "v9 reservation did not receive default check-in diagnostics");
  require(migratedV9.save().starts_with("HHGS 10 "),
          "populated v9 save did not migrate to the v10 writer");

  legacy = with_save_version(legacy, 8);
  for (const auto &review : before.reviews) {
    std::ostringstream currentLine;
    currentLine << std::setprecision(17) << review.reservationId << ' '
                << review.day << ' ' << review.score << ' '
                << std::quoted(review.text);
    std::ostringstream legacyLine;
    legacyLine << review.reservationId << ' ' << review.day << " 80 "
               << std::quoted(review.text);
    const auto position = legacy.find(currentLine.str());
    require(position != std::string::npos,
            "v9 review record was not found in the save");
    legacy.replace(position, currentLine.str().size(), legacyLine.str());
  }
  const auto migrated = Simulation::load(legacy);
  for (const auto &review : migrated.view().reviews)
    require(std::abs(review.score - 8.2) < 1e-12,
            "legacy 0-100 review score did not migrate to 1.0-10.0");
  require(migrated.save().starts_with("HHGS 10 "),
          "populated v8 save did not migrate to the v10 writer");

  const auto &firstReview = before.reviews.front();
  std::ostringstream validLegacyReview;
  validLegacyReview << firstReview.reservationId << ' ' << firstReview.day
                    << " 80 " << std::quoted(firstReview.text);
  std::ostringstream invalidLegacyReview;
  invalidLegacyReview << firstReview.reservationId << ' ' << firstReview.day
                      << " 101 " << std::quoted(firstReview.text);
  auto corrupted = legacy;
  const auto reviewPosition = corrupted.find(validLegacyReview.str());
  require(reviewPosition != std::string::npos,
          "legacy review record was not found in the save");
  corrupted.replace(reviewPosition, validLegacyReview.str().size(),
                    invalidLegacyReview.str());
  bool rejected = false;
  try {
    (void)Simulation::load(corrupted);
  } catch (const std::invalid_argument &) {
    rejected = true;
  }
  require(rejected, "out-of-range legacy review score was accepted");
}

static void construction_and_routes() {
  Simulation s(4, 12, 10, 1);
  require(!s.buildTile({0, -1, 0}, TileKind::Floor), "bounds accepted");
  require(!s.buildFurnishedRoom({"bad", 0, 2, 2, 4, 4, {0, 1, 1}, 1, 1, 100}),
          "door outside room accepted");
  for (int x = 0; x <= 3; ++x)
    require(
        s.buildTile({0, x, 0}, x == 0 ? TileKind::Entrance : TileKind::Floor)
            .ok,
        "corridor");
  auto r = s.buildFurnishedRoom({"101", 0, 2, 1, 4, 4, {0, 2, 1}, 1, 1, 120});
  require(r.ok, "valid room rejected");
  require(s.isReachable({0, 0, 0}, {0, 2, 1}), "room not reachable");
  require(room(s.view(), r.id).beds == 1 && room(s.view(), r.id).baths == 1,
          "not furnished");
}

static void tutorial_contains_explicit_lobby_space() {
  const auto tutorial = Simulation::tutorial(40).view();
  bool lobby = false;
  for (const auto &tile : tutorial.tiles)
    lobby |= tile.kind == TileKind::Lobby;
  require(lobby, "starter property has no explicit lobby space");
}

static void construction_is_atomic_and_budget_limited() {
  Simulation s(5, 30, 30, 1);
  require(s.buildTile({0, 0, 0}, TileKind::Entrance).ok,
          "budget test entrance build failed");
  require(!s.buildTile({0, 1, 0}, TileKind::Entrance),
          "multiple main entrances were accepted");
  const auto before = s.view();
  const auto tooExpensive =
      s.buildFurnishedRoom({"Grand", 0, 1, 1, 20, 20, {0, 1, 1}, 1, 1, 200});
  require(!tooExpensive, "unaffordable room construction was accepted");
  const auto after = s.view();
  require(after.rooms.empty() && after.tiles.size() == before.tiles.size() &&
              after.economy.cashCents == before.economy.cashCents &&
              after.economy.constructionCostCents ==
                  before.economy.constructionCostCents,
          "rejected room construction partially mutated the hotel");
}

static void operational_infrastructure_cannot_strand_service() {
  Simulation incomplete(6, 12, 10, 1);
  for (int x = 0; x < 5; ++x)
    require(
        incomplete
            .buildTile({0, x, 0}, x == 0 ? TileKind::Entrance : TileKind::Floor)
            .ok,
        "infrastructure test corridor build failed");
  require(incomplete
              .buildFurnishedRoom({"101", 0, 2, 1, 3, 3, {0, 2, 1}, 1, 1, 100})
              .ok,
          "infrastructure test room build failed");
  require(incomplete.loadDefinitions(R"({"baseDemand":100})").ok,
          "infrastructure test definitions rejected");
  incomplete.step(15 * 3600);
  require(incomplete.view().reservations.empty(),
          "hotel without reception accepted bookings");

  auto activeDesk = Simulation::tutorial(35);
  require(activeDesk.loadDefinitions(R"({"baseDemand":100})").ok,
          "active-desk test definitions rejected");
  activeDesk.step(3601);
  require(!activeDesk.buildTile({0, 2, 8}, TileKind::Floor),
          "active reception desk was removed during guest service");

  auto activeCloset = Simulation::tutorial(36);
  require(activeCloset.requestClean(activeCloset.view().rooms.front().id).ok,
          "active-closet test cleaning request failed");
  require(!activeCloset.buildTile({0, 7, 8}, TileKind::Floor),
          "supply closet was removed from an active turnover route");
}

static void turnover_resources_and_accounts() {
  auto s = Simulation::tutorial(9);
  auto initial = s.view().economy.cashCents;
  s.step(5 * 86400.0);
  auto v = s.view();
  require(!v.reservations.empty(), "no demand/reservations");
  require(v.economy.revenueCents > 0, "no revenue");
  require(v.economy.payrollCents > 0, "no payroll");
  require(!v.reviews.empty(), "no guest reviews");
  require(v.economy.cashCents != initial, "cash did not change");
  bool sawGuest = false;
  for (auto &p : v.people)
    sawGuest |= p.kind == PersonKind::Guest;
  require(sawGuest, "no actual guests");

  auto dry = Simulation::tutorial(10);
  require(
      dry.loadDefinitions(
             R"({"initialLinen":0,"initialTowels":0,"initialAmenities":0,"initialChemicals":0})")
          .ok,
      "definitions rejected");
  auto target = dry.view().rooms.front().id;
  require(dry.requestClean(target).ok, "clean request rejected");
  dry.step(3600);
  auto d = dry.view();
  bool blocked = false;
  for (auto &t : d.tasks)
    blocked |= t.status == TaskStatus::Blocked && !t.blockedReason.empty();
  require(blocked, "resource shortage did not visibly block turnover");
}

static void deterministic_save_continuation() {
  auto a = Simulation::tutorial(1234);
  a.step(100000);
  auto data = a.save();
  auto b = Simulation::load(data);
  a.step(200000);
  b.step(200000);
  require(a.save() == b.save(), "loaded continuation diverged");
  auto c = Simulation::tutorial(1234);
  c.step(300000);
  require(a.save() == c.save(), "step partition changed deterministic result");
}

static void layout_has_consequences() {
  auto make = [](bool closetNear) {
    Simulation s(88, 32, 12, 1);
    for (int x = 0; x < 29; ++x)
      s.buildTile({0, x, 5}, x == 0 ? TileKind::Entrance : TileKind::Floor);
    s.buildTile({0, closetNear ? 3 : 28, 5}, TileKind::SupplyCloset);
    auto r = s.buildFurnishedRoom({"101", 0, 3, 6, 5, 5, {0, 3, 6}, 1, 1, 120});
    require(r.ok, "layout room build");
    require(s.hireStaff({"Cleaner", PersonKind::Housekeeper, 0, 0, 18}).ok,
            "hire");
    require(s.requestClean(r.id).ok, "clean request");
    return s;
  };
  auto near = make(true), far = make(false);
  near.step(1970);
  far.step(1970);
  auto n = near.view(), f = far.view();
  require(n.rooms.size() == f.rooms.size() &&
              n.people.size() == f.people.size(),
          "layouts not equivalent capacity");
  require(room(n, n.rooms.front().id).status == RoomStatus::VacantReady,
          "near layout did not complete");
  require(room(f, f.rooms.front().id).status != RoomStatus::VacantReady,
          "long supply route had no completion penalty");
}

struct LayoutOutcome {
  std::int64_t guestTravelSeconds{};
  std::int64_t guestWaitSeconds{};
  double guestSatisfaction{};
  int completedStays{};
  int walkedRelocations{};
  std::int64_t operatingProfitCents{};
};

static LayoutOutcome run_layout_campaign(bool efficient) {
  Simulation s(89, 512, 10, 1);
  for (int x = 0; x < 512; ++x)
    require(
        s.buildTile({0, x, 0}, x == 0 ? TileKind::Entrance : TileKind::Floor)
            .ok,
        "layout benchmark corridor build failed");
  require(s.buildTile({0, efficient ? 2 : 510, 0}, TileKind::FrontDesk).ok,
          "layout benchmark desk build failed");
  require(s.buildTile({0, efficient ? 50 : 509, 0}, TileKind::SupplyCloset).ok,
          "layout benchmark closet build failed");
  for (int roomIndex = 0; roomIndex < 6; ++roomIndex) {
    const int x = 4 + roomIndex * 8;
    require(s.buildFurnishedRoom({std::to_string(101 + roomIndex),
                                  0,
                                  x,
                                  1,
                                  3,
                                  3,
                                  {0, x, 1},
                                  1,
                                  1,
                                  140})
                .ok,
            "layout benchmark room build failed");
  }
  require(s.hireStaff({"Desk", PersonKind::Receptionist, 9, 23, 20}).ok,
          "layout benchmark receptionist hire failed");
  require(s.hireStaff({"Rooms", PersonKind::Housekeeper, 8, 20, 18}).ok,
          "layout benchmark housekeeper hire failed");
  for (const auto &room : s.view().rooms)
    require(s.setRoomRate(room.id, 50).ok,
            "layout benchmark launch rate rejected");
  require(
      s.loadDefinitions(
           R"({"baseDemand":100,"initialLinen":200,"initialTowels":400,"initialAmenities":200,"initialChemicals":200})")
          .ok,
      "layout benchmark definitions rejected");

  // Sample every guest ten minutes into the shared arrival rush, before even
  // the least tolerant profile can abandon, then let the campaign play out.
  s.step(15 * 3600 + 10 * 60);
  LayoutOutcome outcome;
  int guests = 0;
  std::set<EntityId> launchReservationIds;
  for (const auto &person : s.view().people)
    if (person.kind == PersonKind::Guest) {
      launchReservationIds.insert(person.reservationId);
      ++guests;
    }
  require(guests == 6, "layout benchmark did not fill equivalent hotels");
  for (const auto &room : s.view().rooms)
    require(s.setRoomRate(room.id, 140).ok,
            "layout benchmark steady rate rejected");
  require(s.loadDefinitions(R"({"baseDemand":0.85})").ok,
          "layout benchmark steady demand rejected");
  const auto countWalked = [](const SimulationView &view) {
    int count = 0;
    for (const auto &reservation : view.reservations)
      count += reservation.walkedRelocated;
    return count;
  };
  // Flush the deliberately discounted launch cohort before measuring the
  // steady-state economics of the two layouts.
  s.step(5 * 86400);
  const auto baseline = s.view();
  s.step(20 * 86400);
  const auto final = s.view();
  const auto economy = final.economy;
  outcome.completedStays =
      economy.completedStays - baseline.economy.completedStays;
  outcome.walkedRelocations = countWalked(final);
  int servedReservations = 0;
  for (const auto &reservation : final.reservations)
    if (launchReservationIds.contains(reservation.id)) {
      outcome.guestTravelSeconds += reservation.checkInTravelSeconds;
      outcome.guestWaitSeconds += reservation.checkInWaitSeconds;
      outcome.guestSatisfaction += reservation.satisfaction;
      ++servedReservations;
    }
  require(servedReservations == 6,
          "layout benchmark lost a launch reservation from history");
  outcome.guestSatisfaction /= servedReservations;
  outcome.operatingProfitCents =
      (economy.revenueCents - baseline.economy.revenueCents) -
      (economy.payrollCents - baseline.economy.payrollCents) -
      (economy.supplyCostCents - baseline.economy.supplyCostCents) -
      (economy.utilityCostCents - baseline.economy.utilityCostCents);
  return outcome;
}

static void poor_layout_lowers_service_quality_and_profit() {
  const auto efficient = run_layout_campaign(true);
  const auto poor = run_layout_campaign(false);
  std::cout << "Layout acceptance (efficient/poor): travel "
            << efficient.guestTravelSeconds << '/' << poor.guestTravelSeconds
            << " s, wait " << efficient.guestWaitSeconds << '/'
            << poor.guestWaitSeconds << " s, satisfaction "
            << efficient.guestSatisfaction << '/' << poor.guestSatisfaction
            << ", stays " << efficient.completedStays << '/'
            << poor.completedStays << ", walks " << efficient.walkedRelocations
            << '/' << poor.walkedRelocations << ", operating profit "
            << efficient.operatingProfitCents << '/'
            << poor.operatingProfitCents << " cents\n";
  require(poor.guestTravelSeconds > efficient.guestTravelSeconds,
          "poor layout did not increase guest travel");
  require(poor.guestWaitSeconds > efficient.guestWaitSeconds,
          "poor layout did not increase completed check-in waits");
  require(poor.guestSatisfaction < efficient.guestSatisfaction,
          "poor layout did not lower guest satisfaction");
  require(poor.completedStays < efficient.completedStays,
          "poor layout did not reduce hotel throughput");
  require(poor.walkedRelocations > efficient.walkedRelocations,
          "poor layout did not cause more physical check-in abandonment");
  require(efficient.operatingProfitCents > 0,
          "efficient benchmark hotel was not operationally viable");
  require(poor.operatingProfitCents < efficient.operatingProfitCents,
          "poor layout did not reduce operating profit");
}

static void excessive_checkin_delays_release_walked_guests() {
  auto s = Simulation::tutorial(90);
  require(s.loadDefinitions(R"({"baseDemand":100})").ok,
          "walked-guest definitions rejected");
  EntityId receptionist = 0;
  for (const auto &person : s.view().people)
    if (person.kind == PersonKind::Receptionist)
      receptionist = person.id;
  require(receptionist && s.fireStaff(receptionist).ok,
          "walked-guest fixture could not remove reception coverage");
  // Observe the release before the 16:00 sales pass can reserve the rooms
  // again for a future arrival.
  s.step(95 * 60);
  const auto view = s.view();
  int walked = 0;
  std::int64_t checkInTravel = 0;
  int checkInWait = 0;
  for (const auto &reservation : view.reservations) {
    walked += reservation.walkedRelocated;
    checkInTravel += reservation.checkInTravelSeconds;
    checkInWait += reservation.checkInWaitSeconds;
  }
  require(walked > 0 && view.economy.completedStays == 0,
          "unserved check-in queue produced no walked guests");
  for (const auto &room : view.rooms)
    require(room.reservationId == 0 && room.status == RoomStatus::VacantReady,
            "walked guest left a room unavailable for resale");
  for (const auto &task : view.tasks)
    require(task.kind != TaskKind::CheckIn ||
                task.status == TaskStatus::Completed,
            "walked guest left an active check-in task");
  const auto saved = s.save();
  const auto restored = Simulation::load(saved).view();
  int restoredWalked = 0;
  std::int64_t restoredCheckInTravel = 0;
  int restoredCheckInWait = 0;
  for (const auto &reservation : restored.reservations) {
    restoredWalked += reservation.walkedRelocated;
    restoredCheckInTravel += reservation.checkInTravelSeconds;
    restoredCheckInWait += reservation.checkInWaitSeconds;
  }
  require(restoredWalked == walked &&
              restoredCheckInTravel == checkInTravel &&
              restoredCheckInWait == checkInWait &&
              Simulation::load(saved).save() == saved,
          "walked reservation did not round-trip exactly");
}

static void construction_preserves_property_invariants() {
  auto s = Simulation::tutorial(15);
  auto r = s.view().rooms.front();
  require(!s.buildTile({0, 0, 8}, TileKind::Floor),
          "entrance destruction accepted");
  require(!s.buildTile({r.floor, r.x + 1, r.y + 1}, TileKind::Floor),
          "room mutation accepted");
  require(!s.isReachable({0, -1, 0}, {0, -1, 0}),
          "same invalid position reachable");
  require(s.buildTile({0, 3, 8}, TileKind::Wall).ok, "corridor edit rejected");
  require(!room(s.view(), r.id).reachable, "room reachability not invalidated");
}

static void invalid_inputs_are_rejected() {
  auto s = Simulation::tutorial(16);
  auto cash = s.view().economy.cashCents;
  require(!s.setRoomRate(s.view().rooms.front().id, NAN),
          "NaN room rate accepted");
  require(!s.orderSupplies({INT_MAX, INT_MAX, INT_MAX, INT_MAX, INT_MAX}),
          "overflowing supply order accepted");
  require(s.view().economy.cashCents == cash, "invalid order changed cash");
  require(!s.loadDefinitions(R"({"baseDemand": 1 garbage})"),
          "malformed JSON accepted");
  require(!s.loadDefinitions(R"({"utilityPerRoomDayCents":1.5})"),
          "fractional smallest-currency utility cost accepted");
  auto saved = s.save();
  auto pos = saved.find("HHGS 10 16 32 20 3");
  require(pos == 0, "unexpected save header");
  saved.replace(std::string("HHGS 10 16 ").size(), 2, "99");
  bool rejected = false;
  try {
    (void)Simulation::load(saved);
  } catch (const std::invalid_argument &) {
    rejected = true;
  }
  require(rejected, "invalid saved dimensions accepted");
  saved = s.save();
  size_t line = 0;
  for (int n = 0; n < 5; ++n)
    line = saved.find('\n', line) + 1;
  auto lineEnd = saved.find('\n', line);
  saved.replace(line, lineEnd - line, "9999999");
  rejected = false;
  try {
    (void)Simulation::load(saved);
  } catch (const std::invalid_argument &) {
    rejected = true;
  }
  require(rejected, "giant saved room count accepted");

  rejected = false;
  try {
    (void)Simulation::load(s.save() + "TRAILING GARBAGE");
  } catch (const std::invalid_argument &) {
    rejected = true;
  }
  require(rejected, "trailing save data accepted");

  saved = s.save();
  const auto firstRoom = s.view().rooms[0];
  const auto secondRoom = s.view().rooms[1];
  const std::string secondRoomRecord =
      std::to_string(secondRoom.id) + " \"" + secondRoom.name + "\"";
  pos = saved.find(secondRoomRecord);
  require(pos != std::string::npos, "second room save record missing");
  saved.replace(pos, std::to_string(secondRoom.id).size(),
                std::to_string(firstRoom.id));
  rejected = false;
  try {
    (void)Simulation::load(saved);
  } catch (const std::invalid_argument &) {
    rejected = true;
  }
  require(rejected, "duplicate global entity ID accepted");

  auto booked = Simulation::tutorial(160);
  require(booked.loadDefinitions(R"({"baseDemand":100})").ok,
          "corrupt-reference test definitions rejected");
  booked.step(3600);
  saved = booked.save();
  const auto reservation = booked.view().reservations.front();
  const std::string reservationRecord = std::to_string(reservation.id) + " \"" +
                                        reservation.guestName + "\" " +
                                        std::to_string(reservation.roomId);
  pos = saved.find(reservationRecord);
  require(pos != std::string::npos, "reservation save record missing");
  pos += reservationRecord.size() - std::to_string(reservation.roomId).size();
  saved.replace(pos, std::to_string(reservation.roomId).size(), "999999");
  rejected = false;
  try {
    (void)Simulation::load(saved);
  } catch (const std::invalid_argument &) {
    rejected = true;
  }
  require(rejected, "reservation reference to missing room accepted");
}

static void completed_tasks_do_not_replay_when_staff_are_fired() {
  auto s = Simulation::tutorial(21);
  auto roomId = s.view().rooms.front().id;
  require(s.requestClean(roomId).ok, "clean request rejected");
  s.step(5 * 3600);
  auto before = s.view();
  EntityId cleaner = 0;
  EntityId completedTask = 0;
  for (const auto &task : before.tasks)
    if (task.kind == TaskKind::Turnover &&
        task.status == TaskStatus::Completed) {
      cleaner = task.employeeId;
      completedTask = task.id;
    }
  require(cleaner && completedTask, "turnover did not complete");
  require(s.fireStaff(cleaner).ok, "cleaner dismissal rejected");
  for (const auto &task : s.view().tasks)
    if (task.id == completedTask)
      require(task.status == TaskStatus::Completed,
              "completed task replayed after dismissal");
}

static void occupied_rooms_cannot_enter_repair_turnover() {
  auto s = Simulation::tutorial(22);
  require(s.loadDefinitions(R"({"baseDemand":100})").ok,
          "occupied-room test definitions rejected");
  s.step(5000);
  EntityId occupied = 0;
  for (const auto &r : s.view().rooms)
    if (r.status == RoomStatus::Occupied)
      occupied = r.id;
  require(occupied, "tutorial guest did not check in");
  require(!s.requestRepair(occupied), "occupied room accepted repair request");
}

static void carried_turnover_supplies_survive_shift_change() {
  Simulation s(23, 16, 10, 1);
  for (int x = 0; x < 12; ++x)
    require(
        s.buildTile({0, x, 3}, x == 0 ? TileKind::Entrance : TileKind::Floor)
            .ok,
        "corridor build failed");
  require(s.buildTile({0, 2, 3}, TileKind::SupplyCloset).ok, "closet failed");
  auto r = s.buildFurnishedRoom({"101", 0, 4, 4, 5, 5, {0, 4, 4}, 1, 1, 100});
  require(r.ok, "room build failed");
  require(
      s.loadDefinitions(
           R"({"baseDemand":0,"turnoverWorkSeconds":4000,"initialLinen":1,"initialTowels":2,"initialAmenities":1,"initialChemicals":1})")
          .ok,
      "definitions failed");
  require(s.hireStaff({"Cleaner", PersonKind::Housekeeper, 0, 1, 18}).ok,
          "cleaner hire failed");
  require(s.requestClean(r.id).ok, "clean request failed");
  s.step(25 * 3600);
  require(room(s.view(), r.id).status == RoomStatus::VacantReady,
          "carried supplies blocked resumed turnover");
}

static void tutorial_campaign_can_operate_profitably() {
  auto s = Simulation::tutorial(24);
  const auto openingCash = s.view().economy.cashCents;
  for (int day = 0; day < 30; ++day) {
    if (s.view().inventory.linen < 12) {
      const auto order = s.orderSupplies({30, 60, 30, 30, 5});
      require(order.ok, "viable tutorial could not fund routine supplies");
    }
    s.step(86400);
  }
  const auto economy = s.view().economy;
  const auto operatingProfit = economy.revenueCents - economy.payrollCents -
                               economy.supplyCostCents -
                               economy.utilityCostCents;
  require(economy.completedStays >= 30,
          "tutorial did not sustain meaningful guest throughput");
  require(operatingProfit > 0, "fully occupied tutorial loses money by design");
  require(economy.cashCents > openingCash && !economy.distressed,
          "successful tutorial cannot finance expansion");
}

static void guest_needs_follow_the_satisfied_score_convention() {
  auto s = Simulation::tutorial(25);
  require(s.view().hour == 14, "tutorial does not open before arrival peak");
  require(s.loadDefinitions(R"({"baseDemand":100})").ok,
          "high-demand test definitions rejected");
  s.step(3600);
  const auto arrived = s.view();
  const PersonView *guest = nullptr;
  for (const auto &person : arrived.people)
    if (person.kind == PersonKind::Guest) {
      guest = &person;
      break;
    }
  require(guest, "no guest arrived during the 15:00 arrival window");
  require(guest->hunger > 80 && guest->rest > 80,
          "guest needs did not begin mostly satisfied");
  const auto hungerBefore = guest->hunger;
  const auto restBefore = guest->rest;
  s.step(60);
  for (const auto &person : s.view().people)
    if (person.id == guest->id) {
      require(person.hunger < hungerBefore,
              "hunger satisfaction increased while awake");
      require(person.rest < restBefore, "energy did not decay while awake");
      return;
    }
  require(false, "arrived guest disappeared");
}

static void checked_in_guests_follow_a_day_night_room_cycle() {
  auto s = Simulation::tutorial(250);
  require(s.loadDefinitions(R"({"baseDemand":100})").ok,
          "guest cycle test definitions rejected");
  s.step(2 * 3600);
  bool awakeInRoom = false;
  for (const auto &person : s.view().people)
    awakeInRoom |=
        person.kind == PersonKind::Guest && person.state == PersonState::Idle;
  require(awakeInRoom, "checked-in daytime guest remained asleep");

  s.step(6 * 3600);
  bool sleeping = false;
  for (const auto &person : s.view().people)
    sleeping |= person.kind == PersonKind::Guest &&
                person.state == PersonState::Sleeping;
  require(sleeping, "in-room guest did not sleep at 22:00");

  s.step(9 * 3600);
  bool awakeNextMorning = false;
  for (const auto &person : s.view().people)
    awakeNextMorning |=
        person.kind == PersonKind::Guest && person.state == PersonState::Idle;
  require(awakeNextMorning, "in-room guest did not wake at 07:00");
}

static void room_price_changes_booking_demand() {
  auto fair = Simulation::tutorial(26);
  auto overpriced = Simulation::tutorial(26);
  require(fair.loadDefinitions(R"({"baseDemand":100})").ok &&
              overpriced.loadDefinitions(R"({"baseDemand":100})").ok,
          "price-demand test definitions rejected");
  for (const auto &roomView : overpriced.view().rooms)
    require(overpriced.setRoomRate(roomView.id, 500).ok,
            "price-demand test could not set room rate");
  fair.step(2 * 86400);
  overpriced.step(2 * 86400);
  require(!fair.view().reservations.empty(),
          "fair rates did not attract high-demand bookings");
  require(overpriced.view().reservations.empty(),
          "rates above the market budget still attracted bookings");
}

static void checkout_requires_physical_reception_service() {
  auto s = Simulation::tutorial(27);
  require(s.loadDefinitions(R"({"baseDemand":100})").ok,
          "checkout test definitions rejected");
  s.step(2 * 3600);
  EntityId receptionist = 0;
  for (const auto &person : s.view().people)
    if (person.kind == PersonKind::Receptionist)
      receptionist = person.id;
  require(receptionist, "checkout test receptionist missing");
  require(s.fireStaff(receptionist).ok,
          "checkout test could not dismiss receptionist");
  s.step(4 * 86400);
  auto withoutService = s.view();
  require(withoutService.economy.completedStays == 0 &&
              withoutService.reviews.empty(),
          "stay completed without physical checkout service");
  bool checkoutWaiting = false;
  for (const auto &task : withoutService.tasks)
    checkoutWaiting |=
        task.kind == TaskKind::CheckOut && task.status != TaskStatus::Completed;
  require(checkoutWaiting,
          "missing checkout task while reception was unmanned");

  require(s.hireStaff({"Relief", PersonKind::Receptionist, 0, 0, 20}).ok,
          "checkout test relief receptionist hire failed");
  s.step(2 * 3600);
  const auto recovered = s.view();
  require(recovered.economy.completedStays > 0 && !recovered.reviews.empty() &&
              recovered.economy.revenueCents > 0,
          "checkout service did not finalize stays and revenue");
}

static void room_commands_preserve_reservations_and_repair_state() {
  auto reserved = Simulation::tutorial(28);
  require(reserved.loadDefinitions(R"({"baseDemand":100})").ok,
          "room lifecycle test definitions rejected");
  reserved.step(3600);
  EntityId reservedRoom = 0;
  for (const auto &roomView : reserved.view().rooms)
    if (roomView.reservationId)
      reservedRoom = roomView.id;
  require(reservedRoom, "room lifecycle test made no reservation");
  require(!reserved.requestClean(reservedRoom),
          "reserved room accepted a cleaning request");
  require(!reserved.closeRoom(reservedRoom, true),
          "reserved room accepted a closure request");

  auto repair = Simulation::tutorial(29);
  const auto roomId = repair.view().rooms.front().id;
  require(repair.requestRepair(roomId).ok,
          "room lifecycle test could not request repair");
  require(!repair.closeRoom(roomId, false),
          "active repair was bypassed by reopening the room");

  auto closed = Simulation::tutorial(30);
  const auto closableId = closed.view().rooms.front().id;
  require(closed.closeRoom(closableId, true).ok,
          "vacant room could not be closed");
  require(room(closed.view(), closableId).closed,
          "player closure is not visible in the simulation view");
  require(closed.closeRoom(closableId, false).ok &&
              !room(closed.view(), closableId).closed,
          "healthy player-closed room could not reopen");

  auto servicing = Simulation::tutorial(31);
  const auto servicedId = servicing.view().rooms.front().id;
  require(servicing.requestClean(servicedId).ok,
          "room lifecycle test could not request cleaning");
  require(!servicing.requestRepair(servicedId),
          "room accepted overlapping cleaning and repair work");
  require(!servicing.removeRoom(servicedId),
          "room with active service work was demolished");
}

static void worn_rooms_create_physical_maintenance_work() {
  auto s = Simulation::tutorial(32);
  require(
      s.loadDefinitions(R"({"baseDemand":0,"roomConditionLossPerDay":100})").ok,
      "maintenance wear definitions rejected");
  const auto partsBefore = s.view().inventory.parts;
  s.step(10 * 3600);
  auto failed = s.view();
  int failedRooms = 0;
  int repairs = 0;
  for (const auto &roomView : failed.rooms)
    failedRooms += roomView.status == RoomStatus::OutOfOrder;
  for (const auto &task : failed.tasks)
    repairs +=
        task.kind == TaskKind::Repair && task.status != TaskStatus::Completed;
  require(failedRooms > 0 && repairs == failedRooms,
          "daily wear did not create one repair task per failed room");

  s.step(12 * 3600);
  auto serviced = s.view();
  bool restored = false;
  for (const auto &roomView : serviced.rooms)
    restored |=
        roomView.condition == 100 && roomView.status != RoomStatus::OutOfOrder;
  require(restored, "maintenance staff did not restore a failed room");
  require(serviced.inventory.parts < partsBefore,
          "repair completed without consuming a spare part");
}

static void payroll_uses_exact_integer_currency_units() {
  Simulation s(33, 8, 8, 1);
  require(s.buildTile({0, 0, 0}, TileKind::Entrance).ok,
          "payroll test entrance build failed");
  require(s.hireStaff({"Exact", PersonKind::Receptionist, 0, 0, 19.99}).ok,
          "payroll test employee hire failed");
  require(s.view().people.front().hourlyWageCents == 1999,
          "hourly wage was not stored in smallest currency units");
  s.step(86400);
  require(s.view().economy.payrollCents == 1999 * 24,
          "fractional-cent wage accrual drifted over one day");
}

static void fatigue_tracks_work_instead_of_idle_shift_time() {
  Simulation s(34, 12, 10, 1);
  for (int x = 0; x < 8; ++x)
    require(
        s.buildTile({0, x, 0}, x == 0 ? TileKind::Entrance : TileKind::Floor)
            .ok,
        "fatigue test corridor build failed");
  require(s.buildTile({0, 2, 0}, TileKind::SupplyCloset).ok,
          "fatigue test closet build failed");
  const auto built =
      s.buildFurnishedRoom({"101", 0, 4, 1, 3, 3, {0, 4, 1}, 1, 1, 100});
  require(built.ok, "fatigue test room build failed");
  const auto hired = s.hireStaff({"Worker", PersonKind::Housekeeper, 0, 8, 18});
  require(hired.ok, "fatigue test employee hire failed");
  s.step(4 * 3600);
  double idleFatigue = -1;
  for (const auto &person : s.view().people)
    if (person.id == hired.id)
      idleFatigue = person.fatigue;
  require(idleFatigue == 0, "idle shift time created employee fatigue");

  require(s.requestClean(built.id).ok, "fatigue test clean request failed");
  s.step(2100);
  double workFatigue = 0;
  for (const auto &person : s.view().people)
    if (person.id == hired.id)
      workFatigue = person.fatigue;
  require(workFatigue > 0, "heavy room work created no fatigue");
  s.step(8 * 3600);
  double recoveredFatigue = -1;
  for (const auto &person : s.view().people)
    if (person.id == hired.id)
      recoveredFatigue = person.fatigue;
  require(recoveredFatigue == 0, "off-duty recovery did not clear fatigue");
}

static void long_campaign_bounds_transient_history() {
  auto s = Simulation::tutorial(37);
  require(
      s.loadDefinitions(
           R"({"baseDemand":100,"turnoverWorkSeconds":1,"checkInWorkSeconds":1,"roomConditionLossPerDay":0,"initialLinen":500,"initialTowels":1000,"initialAmenities":500,"initialChemicals":500})")
          .ok,
      "long-campaign definitions rejected");
  s.step(20 * 86400);
  const auto view = s.view();
  require(view.economy.completedStays > 30,
          "long campaign did not exercise enough guest turnover");
  require(view.tasks.size() <= 128 + view.rooms.size() * 3,
          "completed task history grew without a bound");
  require(view.people.size() <= view.rooms.size() + 3,
          "departed guest actors accumulated in the live world");

  const auto reloaded = Simulation::load(s.save()).view();
  require(reloaded.tasks.size() == view.tasks.size() &&
              reloaded.people.size() == view.people.size() &&
              reloaded.economy.completedStays == view.economy.completedStays,
          "bounded campaign state did not round-trip through the save");
}

int main() {
  try {
    guest_profiles_are_deterministic_and_bounded();
    guest_profiles_propagate_and_round_trip();
    reviews_follow_segment_probability_rules();
    populated_v8_review_scores_migrate_to_ten_point_scale();
    long_campaign_bounds_transient_history();
    payroll_uses_exact_integer_currency_units();
    fatigue_tracks_work_instead_of_idle_shift_time();
    construction_and_routes();
    tutorial_contains_explicit_lobby_space();
    construction_is_atomic_and_budget_limited();
    operational_infrastructure_cannot_strand_service();
    checked_in_guests_follow_a_day_night_room_cycle();
    turnover_resources_and_accounts();
    deterministic_save_continuation();
    layout_has_consequences();
    excessive_checkin_delays_release_walked_guests();
    poor_layout_lowers_service_quality_and_profit();
    construction_preserves_property_invariants();
    invalid_inputs_are_rejected();
    completed_tasks_do_not_replay_when_staff_are_fired();
    occupied_rooms_cannot_enter_repair_turnover();
    carried_turnover_supplies_survive_shift_change();
    tutorial_campaign_can_operate_profitably();
    guest_needs_follow_the_satisfied_score_convention();
    room_price_changes_booking_demand();
    checkout_requires_physical_reception_service();
    room_commands_preserve_reservations_and_repair_state();
    worn_rooms_create_physical_maintenance_work();
  } catch (const std::exception &e) {
    std::cerr << "FAIL: " << e.what() << '\n';
    return 1;
  }
  std::cout << "All simulation behavior tests passed\n";
}
