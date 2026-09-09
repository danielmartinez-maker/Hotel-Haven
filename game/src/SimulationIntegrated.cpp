#include "hh/game/Simulation.h"
#include "hh/game/GuestPsychology.h"
#include "hh/game/GuestPsychologyArchive.h"

#define save saveV11
#define load loadV11
#include "Simulation.cpp"
#undef load
#undef save

#include <iomanip>
#include <sstream>
#include <stdexcept>
#include <unordered_set>

namespace hh::game {
namespace {
constexpr std::string_view final02Marker = "\nFINAL02\n";

int saveVersion(std::string_view data) {
  std::istringstream input{std::string(data)};
  std::string magic;
  int version{};
  input >> magic >> version;
  if (!input || magic != "HHGS")
    throw std::invalid_argument("unsupported simulation save");
  return version;
}
} // namespace

std::string Simulation::save() const {
  std::string base = saveV11();
  if (!base.starts_with("HHGS 11 "))
    throw std::logic_error("legacy save writer did not emit v11 state");
  base.replace(5, 2, "12");

  const auto snapshot = view();
  std::ostringstream appendix;
  appendix << "FINAL02\n" << snapshot.reservations.size() << '\n';
  for (const auto &reservation : snapshot.reservations) {
    const auto psychology = guestPsychology(reservation.id);
    appendix << reservation.id << ' '
             << std::quoted(detail::serializeGuestPsychology(psychology))
             << '\n';
  }
  base += appendix.str();
  return base;
}

Simulation Simulation::load(std::string_view data) {
  if (data.size() > 64 * 1024 * 1024)
    throw std::invalid_argument("simulation save too large");
  const int version = saveVersion(data);
  if (version < 2 || version > 12)
    throw std::invalid_argument("unsupported simulation save");

  const auto marker = data.rfind(final02Marker);
  if (marker == std::string_view::npos) {
    if (version == 12)
      throw std::invalid_argument("missing FINAL-02 save state");
    return loadV11(data);
  }

  std::string legacy(data.substr(0, marker + 1));
  if (legacy.starts_with("HHGS 12 "))
    legacy.replace(5, 2, "11");
  Simulation simulation = loadV11(legacy);

  std::istringstream appendix{
      std::string(data.substr(marker + final02Marker.size()))};
  std::size_t count{};
  appendix >> count;
  if (!appendix || count > 100000)
    throw std::invalid_argument("invalid FINAL-02 guest count");

  std::unordered_set<EntityId> restoredIds;
  for (std::size_t index = 0; index < count; ++index) {
    EntityId guestId{};
    std::string archive;
    appendix >> guestId >> std::quoted(archive);
    if (!appendix || guestId == 0 || !restoredIds.insert(guestId).second)
      throw std::invalid_argument("invalid FINAL-02 guest identity");
    const auto psychology = detail::deserializeGuestPsychology(archive);
    if (psychology.guestId != guestId)
      throw std::invalid_argument("FINAL-02 psychology identity mismatch");

    Reservation *reservation = nullptr;
    for (auto &candidate : simulation.impl_->reservations)
      if (candidate.id == guestId) {
        reservation = &candidate;
        break;
      }
    if (!reservation)
      for (auto &candidate : simulation.impl_->completedReservationHistory)
        if (candidate.id == guestId) {
          reservation = &candidate;
          break;
        }
    if (!reservation || reservation->profile != psychology.profile)
      throw std::invalid_argument("FINAL-02 psychology references missing guest");
    reservation->psychologyArchive = std::move(archive);
  }
  appendix >> std::ws;
  if (!appendix.eof())
    throw std::invalid_argument("unexpected trailing FINAL-02 save data");

  const std::size_t expected = simulation.impl_->reservations.size() +
                               simulation.impl_->completedReservationHistory.size();
  if (version == 12 && restoredIds.size() != expected)
    throw std::invalid_argument("incomplete FINAL-02 psychology save state");
  return simulation;
}

CommandResult Simulation::recordGuestExperience(EntityId guestId,
                                                const ExperienceEvent &event) {
  Reservation *reservation = nullptr;
  for (auto &candidate : impl_->reservations)
    if (candidate.id == guestId) {
      reservation = &candidate;
      break;
    }
  if (!reservation)
    for (auto &candidate : impl_->completedReservationHistory)
      if (candidate.id == guestId) {
        reservation = &candidate;
        break;
      }
  if (!reservation)
    return {false, "Guest psychology state not found", guestId};

  GuestPsychology service(impl_->seed);
  service.restoreGuest(guestPsychology(guestId));
  try {
    service.recordExperience(guestId, event);
    reservation->psychologyArchive = detail::serializeGuestPsychology(
        *service.snapshot(guestId));
  } catch (const std::exception &error) {
    return {false, error.what(), guestId};
  }
  return {true, "Guest experience recorded", guestId};
}

} // namespace hh::game
