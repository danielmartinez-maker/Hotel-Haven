#pragma once

#include "hh/frontend/UiSettings.h"
#include <string>
#include <string_view>

namespace hh::frontend {

[[nodiscard]] std::string serializeUiSettings(const UiSettings& settings);
[[nodiscard]] bool deserializeUiSettings(std::string_view encoded,
                                         UiSettings& settings);

} // namespace hh::frontend
