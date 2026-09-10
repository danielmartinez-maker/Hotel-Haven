#include "hh/game/GuestPsychology.h"
#include <iostream>
#include <stdexcept>

using namespace hh::game;

namespace {
void require(bool value, const char *message) {
  if (!value)
    throw std::runtime_error(message);
}

void expectation_formula_adds_all_documented_inputs_and_clamps() {
  GuestExpectationInputs input;
  input.segmentBase.room = 60;
  input.starClassModifier.room = 10;
  input.reputationModifier.room = 5;
  input.pricePositionModifier.room = 20;
  input.marketingClaimModifier.room = 15;

  input.segmentBase.cleanliness = 20;
  input.starClassModifier.cleanliness = -10;
  input.reputationModifier.cleanliness = -5;
  input.pricePositionModifier.cleanliness = -5;
  input.marketingClaimModifier.cleanliness = -10;

  input.segmentBase.service = 40;
  input.starClassModifier.service = 5;
  input.reputationModifier.service = 5;
  input.pricePositionModifier.service = 5;
  input.marketingClaimModifier.service = 5;

  const auto expectations = calculateGuestExpectations(input);
  require(expectations.room == 100,
          "expectation formula did not clamp category above 100");
  require(expectations.cleanliness == 0,
          "expectation formula did not clamp category below zero");
  require(expectations.service == 60,
          "expectation formula did not add all five documented inputs");
}

void psychology_stores_revalidated_expectation_profile() {
  GuestPsychology psychology(8201);
  GuestProfileView profile;
  psychology.initializeGuest(9201, profile);

  GuestExpectationInputs arrival;
  arrival.segmentBase.room = 65;
  arrival.starClassModifier.room = 8;
  arrival.reputationModifier.room = 4;
  arrival.pricePositionModifier.room = 3;
  arrival.marketingClaimModifier.room = 2;
  arrival.segmentBase.service = 55;
  arrival.reputationModifier.service = 7;

  psychology.updateExpectations(9201, arrival);
  const auto state = psychology.snapshot(9201);
  require(state.has_value(), "expectation fixture guest disappeared");
  require(state->expectations == calculateGuestExpectations(arrival),
          "guest psychology did not retain the authoritative expectation profile");
}
} // namespace

int main() {
  try {
    expectation_formula_adds_all_documented_inputs_and_clamps();
    psychology_stores_revalidated_expectation_profile();
  } catch (const std::exception &error) {
    std::cerr << "FAIL: " << error.what() << '\n';
    return 1;
  }
  std::cout << "All guest expectation tests passed\n";
  return 0;
}
