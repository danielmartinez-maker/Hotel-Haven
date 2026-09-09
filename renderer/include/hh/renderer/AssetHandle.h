#pragma once

#include <cstdint>

namespace hh::renderer {

struct AssetHandle {
    std::uint32_t value{};

    friend constexpr bool operator==(AssetHandle lhs, AssetHandle rhs) noexcept {
        return lhs.value == rhs.value;
    }
};

}  // namespace hh::renderer
