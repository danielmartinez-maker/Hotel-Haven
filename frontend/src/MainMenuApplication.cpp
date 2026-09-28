#include "hh/frontend/MainMenuApplication.h"

namespace hh::frontend {

std::optional<MainMenuGameLaunchIntent>
mainMenuGameLaunchIntent(MainMenuCommand command) {
    switch (command) {
        case MainMenuCommand::StartNewHotel:
            return MainMenuGameLaunchIntent{
                MainMenuGameLaunchMode::NewHotel,
                L"--game --new-hotel"};
        case MainMenuCommand::ContinueLatest:
        case MainMenuCommand::OpenLoadHotel:
            // Persistence currently exposes one authoritative campaign slot.
            return MainMenuGameLaunchIntent{
                MainMenuGameLaunchMode::LoadLatest,
                L"--game --load-save"};
        case MainMenuCommand::None:
        case MainMenuCommand::OpenScenarios:
        case MainMenuCommand::OpenSandbox:
        case MainMenuCommand::OpenSettings:
        case MainMenuCommand::OpenCredits:
        case MainMenuCommand::ExitApplication:
            return std::nullopt;
    }
    return std::nullopt;
}

}  // namespace hh::frontend
