#pragma once
#include "hh/assets/Catalog.h"
#include <map>
#include <set>
#include <string>
#include <string_view>
#include <vector>

namespace hh::assets {
class DependencyGraph {
public:
    static DependencyGraph build(const AssetCatalog& catalog);
    std::vector<std::string> dependencies_of(std::string_view id, bool transitive) const;
    std::vector<std::string> dependents_of(std::string_view id, bool transitive) const;
    std::vector<std::string> topological_order() const;

private:
    std::map<std::string, std::set<std::string, std::less<>>, std::less<>> dependencies_;
    std::map<std::string, std::set<std::string, std::less<>>, std::less<>> dependents_;
};
}
