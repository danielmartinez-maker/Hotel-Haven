#include "hh/frontend/InspectorModel.h"
#include <algorithm>
namespace hh::frontend {
InspectorPresentation InspectorModel::compose(const UiEntitySnapshot& entity) { return {entity.id, entity.kind, entity.title, entity.fields, entity.diagnostics}; }
std::optional<InspectorPresentation> InspectorModel::compose(EntityId id, const std::vector<UiEntitySnapshot>& entities) { const auto it = std::find_if(entities.begin(), entities.end(), [id](const UiEntitySnapshot& entity) { return entity.id == id; }); if (it == entities.end()) return std::nullopt; return compose(*it); }
std::vector<std::string> InspectorModel::textLines(const InspectorPresentation& model) { std::vector<std::string> lines; lines.reserve(1 + model.fields.size() + model.diagnostics.size()); lines.push_back(model.title); for (const auto& field : model.fields) lines.push_back(field.label + ": " + field.value); for (const auto& diagnostic : model.diagnostics) lines.push_back("[" + diagnostic.code + "] " + diagnostic.message); return lines; }
} // namespace hh::frontend
