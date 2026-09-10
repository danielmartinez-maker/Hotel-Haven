#include "hh/game/GuestPsychology.h"
#include <iostream>
#include <stdexcept>

using namespace hh::game;

namespace {
void require(bool value, const char *message) {
  if (!value)
    throw std::runtime_error(message);
}

void guest_sensitivity_modulates_final_category_weight() {
  GuestProfileView highSensitivity;
  highSensitivity.cleanlinessSensitivity = 1.0;
  auto lowSensitivity = highSensitivity;
  lowSensitivity.cleanlinessSensitivity = 0.0;

  GuestPsychology high(8101), low(8101);
  high.initializeGuest(9101, highSensitivity);
  low.initializeGuest(9102, lowSensitivity);

  ExperienceEvent incident;
  incident.type = ExperienceEventType::DirtyBathroom;
  incident.timestampSeconds = 3600;
  incident.category = ExperienceCategory::Cleanliness;
  incident.observedValue = 45;
  incident.expectedValue = 70;
  incident.rawImpact = -20;
  incident.memorySalience = 0;
  high.recordExperience(9101, incident);
  low.recordExperience(9102, incident);

  const auto highState = *high.snapshot(9101);
  const auto lowState = *low.snapshot(9102);
  require(highState.satisfaction.cleanliness ==
              lowState.satisfaction.cleanliness,
          "guest sensitivity changed the attributed event magnitude");
  require(highState.satisfaction.overall < lowState.satisfaction.overall,
          "guest-specific cleanliness sensitivity did not modulate final satisfaction weight");
}
} // namespace

int main() {
  try {
    guest_sensitivity_modulates_final_category_weight();
  } catch (const std::exception &error) {
    std::cerr << "FAIL: " << error.what() << '\n';
    return 1;
  }
  std::cout << "All guest satisfaction tests passed\n";
  return 0;
}
