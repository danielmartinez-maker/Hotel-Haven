#include "hh/frontend/AlertCenter.h"
#include <algorithm>
#include <unordered_map>
#include <unordered_set>
namespace hh::frontend {
void AlertCenter::ingest(const std::vector<AlertSnapshot>& alerts) {
    const auto pruneResolvedHistory = [this] {
        while (resolved_.size() > resolvedHistoryLimit_) {
            const auto evictedId = resolved_.front().id;
            resolved_.erase(resolved_.begin());
            const auto stillActive = std::any_of(active_.begin(), active_.end(), [evictedId](const AlertSnapshot& row) { return row.id == evictedId; });
            if (!stillActive) known_.erase(evictedId);
        }
    };
    const auto storeResolved = [this, &pruneResolvedHistory](AlertSnapshot alert) {
        alert.resolved = true;
        known_[alert.id] = alert;
        const auto existing = std::find_if(resolved_.begin(), resolved_.end(), [&](const AlertSnapshot& row) { return row.id == alert.id; });
        if (existing == resolved_.end()) resolved_.push_back(std::move(alert)); else *existing = std::move(alert);
        pruneResolvedHistory();
    };

    std::unordered_set<std::uint64_t> currentIds;
    std::unordered_set<std::uint64_t> explicitResolvedIds;
    currentIds.reserve(alerts.size());
    explicitResolvedIds.reserve(alerts.size());
    for (const auto& alert : alerts) {
        if (alert.resolved)
            explicitResolvedIds.insert(alert.id);
        else
            currentIds.insert(alert.id);
    }

    std::vector<AlertSnapshot> disappeared;
    for (const auto& active : active_) {
        if (!currentIds.contains(active.id) && !explicitResolvedIds.contains(active.id))
            disappeared.push_back(active);
    }
    active_.erase(std::remove_if(active_.begin(), active_.end(), [&](const AlertSnapshot& row) { return !currentIds.contains(row.id); }), active_.end());
    for (auto& alert : disappeared)
        storeResolved(std::move(alert));

    std::unordered_map<std::uint64_t, std::size_t> activeIndex;
    activeIndex.reserve(active_.size() + alerts.size());
    for (std::size_t index = 0; index < active_.size(); ++index)
        activeIndex.emplace(active_[index].id, index);

    for (const auto& alert : alerts) {
        if (alert.resolved) {
            storeResolved(alert);
            continue;
        }
        known_[alert.id] = alert;
        const auto existing = activeIndex.find(alert.id);
        if (existing == activeIndex.end()) {
            activeIndex.emplace(alert.id, active_.size());
            active_.push_back(alert);
        } else {
            active_[existing->second] = alert;
        }
    }
}
std::vector<AlertSnapshot> AlertCenter::causalChain(std::uint64_t alertId) const { std::vector<AlertSnapshot> reversed; std::unordered_set<std::uint64_t> seen; auto id = alertId; while (id != 0 && seen.insert(id).second) { const auto it = known_.find(id); if (it == known_.end()) break; reversed.push_back(it->second); id = it->second.causalParentId; } std::reverse(reversed.begin(), reversed.end()); return reversed; }
std::optional<UiCommand> AlertCenter::navigationFor(std::uint64_t alertId) const { const auto it = known_.find(alertId); if (it == known_.end() || it->second.sourceEntityId == 0) return std::nullopt; return UiCommand{UiCommandType::OpenInspector, it->second.sourceEntityId}; }
} // namespace hh::frontend
