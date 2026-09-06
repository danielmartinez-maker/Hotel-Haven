#pragma once
#include "hh/assets/Types.h"
#include <cstddef>
#include <span>
#include <string>
#include <vector>

namespace hh::assets {
struct HassetDocument {
    AssetType type{};
    std::string asset_id;
    std::string fingerprint;
    std::vector<std::string> dependencies;
    std::string source_path;
    std::string sidecar_path;
    std::vector<std::byte> payload;
};

std::vector<std::byte> serialize_hasset(const HassetDocument& document);
HassetDocument parse_hasset(std::span<const std::byte> bytes);
}
