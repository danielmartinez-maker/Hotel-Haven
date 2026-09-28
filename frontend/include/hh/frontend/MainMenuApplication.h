#pragma once

#include <cstdint>
#include <optional>
#include <string>

#include "hh/frontend/MainMenuCommands.h"

namespace hh::frontend {

enum class MainMenuGameLaunchMode : std::uint8_t {
    NewHotel,
    LoadLatest,
};

struct MainMenuGameLaunchIntent {
    MainMenuGameLaunchMode mode{MainMenuGameLaunchMode::NewHotel};
    std::wstring arguments;
};

[[nodiscard]] std::optional<MainMenuGameLaunchIntent>
mainMenuGameLaunchIntent(MainMenuCommand command);

}  // namespace hh::frontend
