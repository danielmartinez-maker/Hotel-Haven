#include "hh/frontend/ObjectiveUi.h"
#include <algorithm>
namespace hh::frontend {
void ObjectiveUi::update(const ObjectivesSnapshot& snapshot) { snapshot_ = snapshot; rebuildVisible(); }
bool ObjectiveUi::dismiss(std::uint64_t objectiveId) { const auto it = std::find_if(snapshot_.items.begin(), snapshot_.items.end(), [objectiveId](const ObjectiveSnapshot& item) { return item.id == objectiveId; }); if (it == snapshot_.items.end() || !it->dismissible) return false; dismissed_.insert(objectiveId); rebuildVisible(); return true; }
void ObjectiveUi::rebuildVisible() { visibleItems_.clear(); for (const auto& item : snapshot_.items) if (!dismissed_.contains(item.id)) visibleItems_.push_back(item); }
} // namespace hh::frontend
