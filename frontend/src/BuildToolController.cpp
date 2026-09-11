#include "hh/frontend/BuildCatalog.h"
namespace hh::frontend {
std::optional<UiCommand> BuildToolController::confirmCommand() const { if (!canConfirm()) return std::nullopt; return UiCommand{UiCommandType::BuildConfirm, 0, static_cast<std::int64_t>(preview_.requestId), preview_.rotationQuarterTurns, preview_.itemId}; }
UiCommand BuildToolController::rotateCommand() const { return UiCommand{UiCommandType::BuildRotate, 0, static_cast<std::int64_t>(preview_.requestId), 0, preview_.itemId}; }
UiCommand BuildToolController::cancelCommand() const { return UiCommand{UiCommandType::BuildCancel, 0, static_cast<std::int64_t>(preview_.requestId), 0, preview_.itemId}; }
} // namespace hh::frontend
