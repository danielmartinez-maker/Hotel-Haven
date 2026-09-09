#include "hh/game/GuestPsychology.h"
#include "hh/game/GuestPsychologyArchive.h"
#include <stdexcept>

namespace hh::game {

void GuestPsychology::restoreGuest(const GuestPsychologySnapshot &snapshot) {
  if (snapshot.guestId == 0)
    throw std::invalid_argument("guest psychology restore requires stable id");
  // The archive codec is also the centralized structural validator.
  (void)detail::deserializeGuestPsychology(
      detail::serializeGuestPsychology(snapshot));
  Record record;
  record.snapshot = snapshot;
  guests_[snapshot.guestId] = std::move(record);
}

} // namespace hh::game
