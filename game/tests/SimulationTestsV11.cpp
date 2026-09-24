// FINAL-02 bumps the authoritative save writer from v11 to v12. Keep the
// existing broad simulation regression corpus intact while replacing only the
// version-bound routines with current-format equivalents. The included file
// remains the source of all non-version-specific behavior tests.
#define guest_profiles_propagate_and_round_trip guest_profiles_propagate_and_round_trip_v10
#define populated_v8_review_scores_migrate_to_ten_point_scale populated_v8_review_scores_migrate_to_ten_point_scale_v10
#define invalid_inputs_are_rejected invalid_inputs_are_rejected_v10
#define main simulation_tests_v10_main
#include "SimulationTests.cpp"
#undef main
#undef invalid_inputs_are_rejected
#undef populated_v8_review_scores_migrate_to_ten_point_scale
#undef guest_profiles_propagate_and_round_trip

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
  require(saved.starts_with("HHGS 12 "),
          "FINAL-02 did not bump the save writer to v12");
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
          "v12 guest save is not byte-stable after loading");

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
  require(legacy.starts_with("HHGS 12 "),
          "empty v12 migration fixture failed");
  for (const int version : {11, 10, 9, 8, 7})
    require(Simulation::load(with_save_version(legacy, version))
                .save()
                .starts_with("HHGS 12 "),
            "legacy save did not migrate to the v12 writer");
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
  require(migratedV9.save().starts_with("HHGS 12 "),
          "populated v9 save did not migrate to the v12 writer");

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
  require(migrated.save().starts_with("HHGS 12 "),
          "populated v8 save did not migrate to the v12 writer");

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
  auto pos = saved.find("HHGS 12 16 32 20 3");
  require(pos == 0, "unexpected save header");
  saved.replace(std::string("HHGS 12 16 ").size(), 2, "99");
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
