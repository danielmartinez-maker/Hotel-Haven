#pragma once
#include "hh/frontend/GameUiTypes.h"
#include <optional>
#include <string>
#include <vector>
namespace hh::frontend {
struct InspectorPresentation { EntityId entityId{}; InspectorKind kind{InspectorKind::Room}; std::string title; std::vector<FieldSnapshot> fields; std::vector<DiagnosticSnapshot> diagnostics; };
class InspectorModel {
public:
    [[nodiscard]] static InspectorPresentation compose(const UiEntitySnapshot& entity);
    [[nodiscard]] static std::optional<InspectorPresentation> compose(EntityId id, const std::vector<UiEntitySnapshot>& entities);
    [[nodiscard]] static std::vector<std::string> textLines(const InspectorPresentation& model);
};
} // namespace hh::frontend
