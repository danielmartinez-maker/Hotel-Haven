#include "hh/frontend/AlertCenter.h"
#include <algorithm>
#include <unordered_set>
namespace hh::frontend {
void AlertCenter::ingest(const std::vector<AlertSnapshot>& alerts) {
    for (const auto& alert : alerts) {
        known_[alert.id] = alert;
        if (alert.resolved) {
            active_.erase(std::remove_if(active_.begin(), active_.end(), [&](const AlertSnapshot& row) { return row.id == alert.id; }), active_.end());
            const auto existing = std::find_if(resolved_.begin(), resolved_.end(), [&](const AlertSnapshot& row) { return row.id == alert.id; });
            if (existing == resolved_.end()) resolved_.push_back(alert); else *existing = alert;
            while (resolved_.size() > resolvedHistoryLimit_) {
                const auto evictedId = resolved_.front().id;
                resolved_.erase(resolved_.begin());
                const auto stillActive = std::any_of(active_.begin(), active_.end(), [evictedId](const AlertSnapshot& row) { return row.id == evictedId; });
                if (!stillActive) known_.erase(evictedId);
            }
        } else {
            const auto existing = std::find_if(active_.begin(), active_.end(), [&](const AlertSnapshot& row) { return row.id == alert.id; });
            if (existing == active_.end()) active_.push_back(alert); else *existing = alert;
        }
    }
}
std::vector<AlertSnapshot> AlertCenter::causalChain(std::uint64_t alertId) const { std::vector<AlertSnapshot> reversed; std::unordered_set<std::uint64_t> seen; auto id = alertId; while (id != 0 && seen.insert(id).second) { const auto it = known_.find(id); if (it == known_.end()) break; reversed.push_back(it->second); id = it->second.causalParentId; } std::reverse(reversed.begin(), reversed.end()); return reversed; }
std::optional<UiCommand> AlertCenter::navigationFor(std::uint64_t alertId) const { const auto it = known_.find(alertId); if (it == known_.end() || it->second.sourceEntityId == 0) return std::nullopt; return UiCommand{UiCommandType::OpenInspector, it->second.sourceEntityId}; }
} // namespace hh::frontend
