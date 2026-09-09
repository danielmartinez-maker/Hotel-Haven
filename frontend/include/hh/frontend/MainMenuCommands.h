#pragma once

#include <cstdint>

namespace hh::frontend {

enum class MainMenuItem : std::uint8_t {
    Continue,
    NewHotel,
    LoadHotel,
    Scenarios,
    Sandbox,
    Settings,
    Credits,
    Quit,
};

enum class MainMenuCommand : std::uint8_t {
    None,
    ContinueLatest,
    StartNewHotel,
    OpenLoadHotel,
    OpenScenarios,
    OpenSandbox,
    OpenSettings,
    OpenCredits,
    ExitApplication,
};

enum class MainMenuModal : std::uint8_t {
    None,
    QuitConfirm,
};

}  // namespace hh::frontend
