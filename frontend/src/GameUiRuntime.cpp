#include "hh/frontend/GameUiRuntime.h"
#include <utility>
namespace hh::frontend {
GameUiSnapshot buildGameUiSnapshot(const SimulationSnapshot& source) { GameUiSnapshot result; result.revision = source.revision; result.hud = source.hud; result.entities = source.entities; result.buildCatalog = source.buildCatalog; result.buildPreview = source.buildPreview; result.overlays = source.overlays; result.operations = source.operations; result.economy = source.economy; result.alerts = source.alerts; result.objectives = source.objectives; return result; }
GameUiRuntime::GameUiRuntime(CommandSink commandSink) : commandSink_(std::move(commandSink)) {}
void GameUiRuntime::update(const SimulationSnapshot& source) { if (snapshotBuildCount_ != 0 && source.revision == snapshot_.revision) return; snapshot_ = buildGameUiSnapshot(source); ++snapshotBuildCount_; }
UiCommandResult GameUiRuntime::dispatchUiCommand(const UiCommand& command) { if (!commandSink_) return {false, "UI_COMMAND_NO_DISPATCHER", "No application command dispatcher is connected"}; return commandSink_(command); }
} // namespace hh::frontend
