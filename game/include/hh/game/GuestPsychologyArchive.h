#pragma once

#include "hh/game/GuestPsychology.h"
#include <string>
#include <string_view>

namespace hh::game::detail {

[[nodiscard]] std::string
serializeGuestPsychology(const GuestPsychologySnapshot &snapshot);
[[nodiscard]] GuestPsychologySnapshot
deserializeGuestPsychology(std::string_view serialized);

} // namespace hh::game::detail
