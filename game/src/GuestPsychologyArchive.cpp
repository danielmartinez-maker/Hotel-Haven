#include "hh/game/GuestPsychologyArchive.h"
#include <cmath>
#include <iomanip>
#include <sstream>
#include <stdexcept>

namespace hh::game::detail {
namespace {
template <class E> int enumValue(E value) noexcept {
  return static_cast<int>(value);
}

bool bounded100(int value) noexcept { return value >= 0 && value <= 100; }
bool bounded10000(int value) noexcept { return value >= 0 && value <= 10000; }

void writeProfile(std::ostream &output, const GuestProfileView &profile) {
  output << enumValue(profile.archetype) << ' ' << profile.budgetPerNightCents
         << ' ' << profile.priceSensitivity << ' ' << profile.serviceSensitivity
         << ' ' << profile.cleanlinessSensitivity << ' '
         << profile.noiseSensitivity << ' ' << profile.privacySensitivity << ' '
         << profile.safetySensitivity << ' ' << profile.comfortSensitivity << ' '
         << profile.foodSensitivity << ' ' << profile.patience << ' '
         << profile.traitFlags;
}

GuestProfileView readProfile(std::istream &input) {
  GuestProfileView profile;
  int archetype{};
  input >> archetype >> profile.budgetPerNightCents >> profile.priceSensitivity >>
      profile.serviceSensitivity >> profile.cleanlinessSensitivity >>
      profile.noiseSensitivity >> profile.privacySensitivity >>
      profile.safetySensitivity >> profile.comfortSensitivity >>
      profile.foodSensitivity >> profile.patience >> profile.traitFlags;
  profile.archetype = static_cast<GuestArchetype>(archetype);
  if (!input || !GuestPsychology::validProfile(profile))
    throw std::invalid_argument("invalid archived guest profile");
  return profile;
}

void validateSnapshot(const GuestPsychologySnapshot &snapshot) {
  if (snapshot.guestId == 0 || !GuestPsychology::validProfile(snapshot.profile))
    throw std::invalid_argument("invalid archived guest psychology identity");
  for (int value : {snapshot.needs.energy, snapshot.needs.hunger,
                    snapshot.needs.hygiene, snapshot.needs.comfort,
                    snapshot.needs.entertainment, snapshot.needs.social,
                    snapshot.needs.privacy, snapshot.needs.safety,
                    snapshot.operational.serviceConfidence,
                    snapshot.operational.cleanlinessConfidence,
                    snapshot.operational.environmentComfort,
                    snapshot.operational.valuePerception,
                    snapshot.expectations.room,
                    snapshot.expectations.cleanliness,
                    snapshot.expectations.service, snapshot.expectations.food,
                    snapshot.expectations.amenities, snapshot.expectations.quiet,
                    snapshot.expectations.convenience,
                    snapshot.expectations.value, snapshot.satisfaction.room,
                    snapshot.satisfaction.service,
                    snapshot.satisfaction.cleanliness,
                    snapshot.satisfaction.food,
                    snapshot.satisfaction.amenities,
                    snapshot.satisfaction.convenience,
                    snapshot.satisfaction.value,
                    snapshot.satisfaction.arrivalDeparture,
                    snapshot.satisfaction.checkIn, snapshot.satisfaction.noise,
                    snapshot.satisfaction.waits,
                    snapshot.satisfaction.expectationFit,
                    snapshot.satisfaction.overall})
    if (!bounded100(value))
      throw std::invalid_argument("archived guest psychology value out of range");
  for (int value : {snapshot.preferences.wifi, snapshot.preferences.desk,
                    snapshot.preferences.breakfast, snapshot.preferences.spa,
                    snapshot.preferences.pool, snapshot.preferences.fitness,
                    snapshot.preferences.social, snapshot.preferences.quietRoom,
                    snapshot.preferences.roomQuality})
    if (!bounded10000(value))
      throw std::invalid_argument("archived guest preference out of range");
  if (snapshot.memories.size() > 10000 || snapshot.complaints.size() > 10000)
    throw std::invalid_argument("archived guest psychology history too large");
  for (const auto &memory : snapshot.memories)
    if (enumValue(memory.type) < enumValue(ExperienceEventType::FastCheckIn) ||
        enumValue(memory.type) >
            enumValue(ExperienceEventType::StaffExceptionalService) ||
        enumValue(memory.category) < enumValue(ExperienceCategory::Room) ||
        enumValue(memory.category) >
            enumValue(ExperienceCategory::ArrivalDeparture) ||
        memory.timestampSeconds < 0 || memory.valence < -1 ||
        memory.valence > 1 || memory.magnitude < 0 || memory.magnitude > 100 ||
        memory.salience < 0 || memory.salience > 10000 ||
        memory.halfLifeHours < 1 || memory.halfLifeHours > 24 * 3650)
      throw std::invalid_argument("invalid archived guest memory");
  for (const auto &complaint : snapshot.complaints)
    if (enumValue(complaint.type) < enumValue(ComplaintType::CheckInDelay) ||
        enumValue(complaint.type) > enumValue(ComplaintType::ServiceFailure) ||
        enumValue(complaint.urgency) < enumValue(ComplaintUrgency::Low) ||
        enumValue(complaint.urgency) > enumValue(ComplaintUrgency::Critical) ||
        enumValue(complaint.sourceEvent) <
            enumValue(ExperienceEventType::FastCheckIn) ||
        enumValue(complaint.sourceEvent) >
            enumValue(ExperienceEventType::StaffExceptionalService) ||
        complaint.timestampSeconds < 0 || complaint.magnitude < 0 ||
        complaint.magnitude > 100)
      throw std::invalid_argument("invalid archived guest complaint");
}
} // namespace

std::string serializeGuestPsychology(const GuestPsychologySnapshot &snapshot) {
  validateSnapshot(snapshot);
  std::ostringstream output;
  output << std::setprecision(17) << snapshot.guestId << ' ';
  writeProfile(output, snapshot.profile);
  output << ' ' << snapshot.needs.energy << ' ' << snapshot.needs.hunger << ' '
         << snapshot.needs.hygiene << ' ' << snapshot.needs.comfort << ' '
         << snapshot.needs.entertainment << ' ' << snapshot.needs.social << ' '
         << snapshot.needs.privacy << ' ' << snapshot.needs.safety << ' '
         << snapshot.preferences.wifi << ' ' << snapshot.preferences.desk << ' '
         << snapshot.preferences.breakfast << ' ' << snapshot.preferences.spa
         << ' ' << snapshot.preferences.pool << ' '
         << snapshot.preferences.fitness << ' ' << snapshot.preferences.social
         << ' ' << snapshot.preferences.quietRoom << ' '
         << snapshot.preferences.roomQuality << ' '
         << snapshot.operational.serviceConfidence << ' '
         << snapshot.operational.cleanlinessConfidence << ' '
         << snapshot.operational.environmentComfort << ' '
         << snapshot.operational.valuePerception << ' '
         << snapshot.expectations.room << ' '
         << snapshot.expectations.cleanliness << ' '
         << snapshot.expectations.service << ' ' << snapshot.expectations.food
         << ' ' << snapshot.expectations.amenities << ' '
         << snapshot.expectations.quiet << ' '
         << snapshot.expectations.convenience << ' '
         << snapshot.expectations.value << ' ' << snapshot.satisfaction.room
         << ' ' << snapshot.satisfaction.service << ' '
         << snapshot.satisfaction.cleanliness << ' '
         << snapshot.satisfaction.food << ' ' << snapshot.satisfaction.amenities
         << ' ' << snapshot.satisfaction.convenience << ' '
         << snapshot.satisfaction.value << ' '
         << snapshot.satisfaction.arrivalDeparture << ' '
         << snapshot.satisfaction.checkIn << ' ' << snapshot.satisfaction.noise
         << ' ' << snapshot.satisfaction.waits << ' '
         << snapshot.satisfaction.expectationFit << ' '
         << snapshot.satisfaction.overall << ' ' << snapshot.memories.size();
  for (const auto &memory : snapshot.memories)
    output << ' ' << enumValue(memory.type) << ' ' << memory.timestampSeconds
           << ' ' << memory.locationId << ' ' << memory.sourceEntityId << ' '
           << enumValue(memory.category) << ' ' << memory.valence << ' '
           << memory.magnitude << ' ' << memory.salience << ' '
           << memory.halfLifeHours << ' ' << memory.resolved;
  output << ' ' << snapshot.complaints.size();
  for (const auto &complaint : snapshot.complaints)
    output << ' ' << enumValue(complaint.type) << ' '
           << enumValue(complaint.urgency) << ' ' << complaint.timestampSeconds
           << ' ' << complaint.locationId << ' ' << complaint.sourceEntityId
           << ' ' << enumValue(complaint.sourceEvent) << ' '
           << complaint.magnitude << ' ' << complaint.resolved;
  return output.str();
}

GuestPsychologySnapshot deserializeGuestPsychology(std::string_view serialized) {
  std::istringstream input{std::string(serialized)};
  GuestPsychologySnapshot snapshot;
  input >> snapshot.guestId;
  snapshot.profile = readProfile(input);
  input >> snapshot.needs.energy >> snapshot.needs.hunger >>
      snapshot.needs.hygiene >> snapshot.needs.comfort >>
      snapshot.needs.entertainment >> snapshot.needs.social >>
      snapshot.needs.privacy >> snapshot.needs.safety >>
      snapshot.preferences.wifi >> snapshot.preferences.desk >>
      snapshot.preferences.breakfast >> snapshot.preferences.spa >>
      snapshot.preferences.pool >> snapshot.preferences.fitness >>
      snapshot.preferences.social >> snapshot.preferences.quietRoom >>
      snapshot.preferences.roomQuality >>
      snapshot.operational.serviceConfidence >>
      snapshot.operational.cleanlinessConfidence >>
      snapshot.operational.environmentComfort >>
      snapshot.operational.valuePerception >> snapshot.expectations.room >>
      snapshot.expectations.cleanliness >> snapshot.expectations.service >>
      snapshot.expectations.food >> snapshot.expectations.amenities >>
      snapshot.expectations.quiet >> snapshot.expectations.convenience >>
      snapshot.expectations.value >> snapshot.satisfaction.room >>
      snapshot.satisfaction.service >> snapshot.satisfaction.cleanliness >>
      snapshot.satisfaction.food >> snapshot.satisfaction.amenities >>
      snapshot.satisfaction.convenience >> snapshot.satisfaction.value >>
      snapshot.satisfaction.arrivalDeparture >> snapshot.satisfaction.checkIn >>
      snapshot.satisfaction.noise >> snapshot.satisfaction.waits >>
      snapshot.satisfaction.expectationFit >> snapshot.satisfaction.overall;
  std::size_t memoryCount{};
  input >> memoryCount;
  if (!input || memoryCount > 10000)
    throw std::invalid_argument("invalid archived guest memory count");
  snapshot.memories.resize(memoryCount);
  for (auto &memory : snapshot.memories) {
    int type{}, category{};
    input >> type >> memory.timestampSeconds >> memory.locationId >>
        memory.sourceEntityId >> category >> memory.valence >> memory.magnitude >>
        memory.salience >> memory.halfLifeHours >> memory.resolved;
    memory.type = static_cast<ExperienceEventType>(type);
    memory.category = static_cast<ExperienceCategory>(category);
  }
  std::size_t complaintCount{};
  input >> complaintCount;
  if (!input || complaintCount > 10000)
    throw std::invalid_argument("invalid archived guest complaint count");
  snapshot.complaints.resize(complaintCount);
  for (auto &complaint : snapshot.complaints) {
    int type{}, urgency{}, sourceEvent{};
    input >> type >> urgency >> complaint.timestampSeconds >>
        complaint.locationId >> complaint.sourceEntityId >> sourceEvent >>
        complaint.magnitude >> complaint.resolved;
    complaint.type = static_cast<ComplaintType>(type);
    complaint.urgency = static_cast<ComplaintUrgency>(urgency);
    complaint.sourceEvent = static_cast<ExperienceEventType>(sourceEvent);
  }
  input >> std::ws;
  if (!input.eof())
    throw std::invalid_argument("unexpected trailing archived psychology data");
  validateSnapshot(snapshot);
  return snapshot;
}

} // namespace hh::game::detail
