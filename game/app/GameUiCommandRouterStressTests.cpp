#include "GameUiCommandRouter.h"

#include <array>
#include <cstdint>
#include <iostream>
#include <stdexcept>
#include <string>

namespace {
using namespace hh::frontend;

void require(bool value, const char *message) {
  if (!value)
    throw std::runtime_error(message);
}

struct AuthoritativeFixture {
  std::int64_t speed{1};
  std::uint64_t commandVersion{};
  std::int64_t futureRateCents{15'000};
  std::int64_t overbookingAllowance{2};
};

std::uint64_t hashFixture(const AuthoritativeFixture &fixture) {
  std::uint64_t hash = 1469598103934665603ULL;
  const auto mix = [&](std::uint64_t value) mutable {
    hash ^= value;
    hash *= 1099511628211ULL;
  };
  mix(static_cast<std::uint64_t>(fixture.speed));
  mix(fixture.commandVersion);
  mix(static_cast<std::uint64_t>(fixture.futureRateCents));
  mix(static_cast<std::uint64_t>(fixture.overbookingAllowance));
  return hash;
}

bool isAuthorityCommand(UiCommandType type) {
  switch (type) {
  case UiCommandType::BuildConfirm:
  case UiCommandType::BuildRotate:
  case UiCommandType::BuildCancel:
  case UiCommandType::SetFutureRate:
  case UiCommandType::SetOverbookingPolicy:
  case UiCommandType::StartMarketingCampaign:
  case UiCommandType::AcceptContract:
    return true;
  default:
    return false;
  }
}
} // namespace

int main() {
  try {
    AuthoritativeFixture fixture;
    std::uint64_t speedCalls = 0;
    std::uint64_t saveCalls = 0;
    std::uint64_t loadCalls = 0;
    std::uint64_t settingsCalls = 0;
    std::uint64_t inspectorCalls = 0;
    std::uint64_t panelCalls = 0;
    std::uint64_t overlayCalls = 0;
    std::uint64_t authorityCalls = 0;
    std::uint64_t authorityRejects = 0;

    hh::client::GameUiCommandHooks hooks;
    hooks.setSimulationSpeed = [&](SimulationSpeed speed) {
      ++speedCalls;
      fixture.speed = static_cast<std::int64_t>(speed);
      return UiCommandResult{true, {}, {}};
    };
    hooks.saveGame = [&] {
      ++saveCalls;
      return UiCommandResult{true, {}, "saved"};
    };
    hooks.loadGame = [&] {
      ++loadCalls;
      return UiCommandResult{true, {}, "loaded"};
    };
    hooks.openSettings = [&] {
      ++settingsCalls;
      return UiCommandResult{true, {}, {}};
    };
    hooks.openInspector = [&](EntityId) { ++inspectorCalls; };
    hooks.openManagementPanel = [&](ManagementPanelId) { ++panelCalls; };
    hooks.setOverlay = [&](OverlayId) {
      ++overlayCalls;
      return UiCommandResult{true, {}, {}};
    };
    hooks.authorityCommand = [&](const UiCommand &command) {
      ++authorityCalls;
      if (command.integerValue < 0) {
        ++authorityRejects;
        return UiCommandResult{false, "AUTH_STALE", "stale or invalid authority command"};
      }
      ++fixture.commandVersion;
      if (command.type == UiCommandType::SetFutureRate)
        fixture.futureRateCents = command.integerValue;
      if (command.type == UiCommandType::SetOverbookingPolicy)
        fixture.overbookingAllowance = command.integerValue;
      return UiCommandResult{true, {}, {}};
    };

    constexpr std::array<std::int64_t, 5> validSpeeds{0, 1, 2, 4, 8};
    constexpr std::uint64_t kAttempts = 100'000;
    std::uint64_t routerRejects = 0;

    for (std::uint64_t i = 0; i < kAttempts; ++i) {
      UiCommand command;
      switch (i % 15) {
      case 0:
        command = {UiCommandType::SetSimulationSpeed, 0,
                   validSpeeds[static_cast<std::size_t>(i % validSpeeds.size())]};
        break;
      case 1:
        command = {UiCommandType::SetSimulationSpeed, 0, 3};
        break;
      case 2:
        command = {UiCommandType::PauseGame};
        break;
      case 3:
        command = {UiCommandType::OpenInspector, static_cast<EntityId>(1 + i % 10'000)};
        break;
      case 4:
        command = {UiCommandType::OpenManagementPanel, 0,
                   static_cast<std::int64_t>(i % (static_cast<std::uint64_t>(ManagementPanelId::Supplies) + 1))};
        break;
      case 5:
        command = {UiCommandType::OpenManagementPanel, 0, 999};
        break;
      case 6:
        command = {UiCommandType::SetOverlay, 0,
                   static_cast<std::int64_t>(i % (static_cast<std::uint64_t>(OverlayId::Revenue) + 1))};
        break;
      case 7:
        command = {UiCommandType::SetOverlay, 0, -1};
        break;
      case 8:
        command = {UiCommandType::SaveGame};
        break;
      case 9:
        command = {UiCommandType::LoadGame};
        break;
      case 10:
        command = {UiCommandType::OpenSettings};
        break;
      case 11:
        command = {UiCommandType::SetFutureRate, 0,
                   (i & 1ULL) ? -1 : static_cast<std::int64_t>(10'000 + i % 20'000),
                   static_cast<std::int64_t>(i % 365), "standard"};
        break;
      case 12:
        command = {UiCommandType::SetOverbookingPolicy, 0,
                   (i & 1ULL) ? -1 : static_cast<std::int64_t>(i % 8), 0,
                   "standard"};
        break;
      case 13:
        command = {UiCommandType::BuildConfirm, 0,
                   (i & 1ULL) ? -1 : static_cast<std::int64_t>(i + 1),
                   static_cast<std::int64_t>(i % 4), "guest_bed"};
        break;
      case 14:
        command = {UiCommandType::None};
        break;
      }

      const auto beforeHash = hashFixture(fixture);
      const auto speedBefore = speedCalls;
      const auto panelBefore = panelCalls;
      const auto overlayBefore = overlayCalls;
      const auto authorityBefore = authorityCalls;
      const auto result = hh::client::routeGameUiCommand(command, hooks);

      if (!result.ok) {
        ++routerRejects;
        require(hashFixture(fixture) == beforeHash,
                "rejected UI command mutated authoritative fixture");
      }
      if (command.type == UiCommandType::SetSimulationSpeed && command.integerValue == 3) {
        require(!result.ok && result.reasonCode == "UI_SPEED_INVALID",
                "invalid speed did not preserve stable rejection reason");
        require(speedCalls == speedBefore,
                "invalid speed reached authoritative speed hook");
      }
      if (command.type == UiCommandType::OpenManagementPanel && command.integerValue == 999) {
        require(!result.ok && result.reasonCode == "UI_PANEL_INVALID",
                "invalid panel did not preserve stable rejection reason");
        require(panelCalls == panelBefore,
                "invalid panel reached navigation hook");
      }
      if (command.type == UiCommandType::SetOverlay && command.integerValue == -1) {
        require(!result.ok && result.reasonCode == "UI_OVERLAY_INVALID",
                "invalid overlay did not preserve stable rejection reason");
        require(overlayCalls == overlayBefore,
                "invalid overlay reached presentation hook");
      }
      if (command.type == UiCommandType::None)
        require(!result.ok && result.reasonCode == "UI_COMMAND_EMPTY",
                "empty command did not preserve stable rejection reason");
      if (isAuthorityCommand(command.type)) {
        require(authorityCalls == authorityBefore + 1,
                "typed authority command was not forwarded exactly once");
        if (command.integerValue < 0)
          require(!result.ok && result.reasonCode == "AUTH_STALE",
                  "authoritative stale rejection was not propagated verbatim");
      }
    }

    require(speedCalls > 0 && saveCalls > 0 && loadCalls > 0 && settingsCalls > 0,
            "stress campaign missed application command families");
    require(inspectorCalls > 0 && panelCalls > 0 && overlayCalls > 0,
            "stress campaign missed UI navigation families");
    require(authorityCalls > 0 && authorityRejects > 0 && routerRejects > 0,
            "stress campaign missed authority/rejection paths");

    std::cout << "StressUiCommandRouter PASS attempts=" << kAttempts
              << " routerRejects=" << routerRejects
              << " authorityRejects=" << authorityRejects << '\n';
    return 0;
  } catch (const std::exception &error) {
    std::cerr << "StressUiCommandRouter FAIL: " << error.what() << '\n';
    return 1;
  }
}
