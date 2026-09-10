#include "GameUiCommandRouter.h"
#include <iostream>
#include <stdexcept>

namespace {
void require(bool value, const char* message) {
  if (!value) throw std::runtime_error(message);
}
}

int main() {
  try {
    using namespace hh::frontend;
    int speedCalls = 0;
    int saveCalls = 0;
    EntityId inspected = 0;
    UiCommandType forwarded = UiCommandType::None;

    hh::client::GameUiCommandHooks hooks;
    hooks.setSimulationSpeed = [&](SimulationSpeed speed) {
      ++speedCalls;
      require(speed == SimulationSpeed::FourX, "wrong speed reached application hook");
      return UiCommandResult{true, {}, {}};
    };
    hooks.saveGame = [&] {
      ++saveCalls;
      return UiCommandResult{true, {}, "saved"};
    };
    hooks.openInspector = [&](EntityId id) { inspected = id; };
    hooks.authorityCommand = [&](const UiCommand& command) {
      forwarded = command.type;
      return UiCommandResult{false, "AUTH_REJECT", "authoritative rejection"};
    };

    auto result = hh::client::routeGameUiCommand(
        UiCommand{UiCommandType::SetSimulationSpeed, 0, 4}, hooks);
    require(result.ok && speedCalls == 1, "valid speed did not route");

    result = hh::client::routeGameUiCommand(
        UiCommand{UiCommandType::SetSimulationSpeed, 0, 3}, hooks);
    require(!result.ok && result.reasonCode == "UI_SPEED_INVALID",
            "invalid speed was not rejected before application hook");
    require(speedCalls == 1, "invalid speed reached application hook");

    result = hh::client::routeGameUiCommand(UiCommand{UiCommandType::SaveGame}, hooks);
    require(result.ok && saveCalls == 1, "save did not route through application hook");

    result = hh::client::routeGameUiCommand(
        UiCommand{UiCommandType::OpenInspector, 42}, hooks);
    require(result.ok && inspected == 42, "inspector navigation did not route");

    result = hh::client::routeGameUiCommand(
        UiCommand{UiCommandType::SetFutureRate, 0, 18999, 21, "deluxe"}, hooks);
    require(!result.ok && result.reasonCode == "AUTH_REJECT" &&
                forwarded == UiCommandType::SetFutureRate,
            "authoritative economy command was not forwarded verbatim");

    std::cout << "FINAL-07 UI command router passed\n";
  } catch (const std::exception& error) {
    std::cerr << error.what() << '\n';
    return 1;
  }
}
