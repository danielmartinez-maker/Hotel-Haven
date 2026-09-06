#include "hh/assets/DependencyGraph.h"
#include <functional>
#include <queue>
#include <stdexcept>

namespace hh::assets {
DependencyGraph DependencyGraph::build(const AssetCatalog& catalog) {
    DependencyGraph graph;
    for (const auto& [id, record] : catalog.records()) {
        graph.dependencies_[id];
        graph.dependents_[id];
        for (const auto& dep : record.metadata.dependencies) {
            if (catalog.records().find(dep) == catalog.records().end()) {
                throw std::runtime_error("missing dependency " + dep + " required by " + id);
            }
            graph.dependencies_[id].insert(dep);
            graph.dependents_[dep].insert(id);
        }
    }
    static_cast<void>(graph.topological_order());
    return graph;
}

namespace {
std::vector<std::string> traverse(
    const std::map<std::string, std::set<std::string, std::less<>>, std::less<>>& adjacency,
    std::string_view id,
    bool transitive) {
    const auto start = adjacency.find(id);
    if (start == adjacency.end()) throw std::out_of_range("unknown asset_id: " + std::string(id));
    if (!transitive) return {start->second.begin(), start->second.end()};
    std::set<std::string, std::less<>> visited;
    std::function<void(const std::string&)> visit = [&](const std::string& current) {
        const auto it = adjacency.find(current);
        if (it == adjacency.end()) return;
        for (const auto& next : it->second) {
            if (visited.insert(next).second) visit(next);
        }
    };
    visit(std::string(id));
    return {visited.begin(), visited.end()};
}
}

std::vector<std::string> DependencyGraph::dependencies_of(std::string_view id, bool transitive) const {
    return traverse(dependencies_, id, transitive);
}
std::vector<std::string> DependencyGraph::dependents_of(std::string_view id, bool transitive) const {
    return traverse(dependents_, id, transitive);
}

std::vector<std::string> DependencyGraph::topological_order() const {
    std::map<std::string, std::size_t, std::less<>> remaining_dependencies;
    for (const auto& [id, deps] : dependencies_) remaining_dependencies[id] = deps.size();
    std::priority_queue<std::string, std::vector<std::string>, std::greater<>> ready;
    for (const auto& [id, count] : remaining_dependencies) if (count == 0) ready.push(id);
    std::vector<std::string> order;
    while (!ready.empty()) {
        auto id = ready.top(); ready.pop();
        order.push_back(id);
        const auto dependent_it = dependents_.find(id);
        if (dependent_it == dependents_.end()) continue;
        for (const auto& dependent : dependent_it->second) {
            auto count_it = remaining_dependencies.find(dependent);
            if (count_it == remaining_dependencies.end() || count_it->second == 0) continue;
            --count_it->second;
            if (count_it->second == 0) ready.push(dependent);
        }
    }
    if (order.size() != dependencies_.size()) throw std::runtime_error("asset dependency cycle detected");
    return order;
}
}
