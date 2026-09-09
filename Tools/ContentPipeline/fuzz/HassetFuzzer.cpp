#include "hh/assets/Hasset.h"

#include <cstddef>
#include <cstdint>
#include <span>
#include <stdexcept>

extern "C" int LLVMFuzzerTestOneInput(const std::uint8_t* data, std::size_t size) {
    const auto bytes = std::span<const std::byte>(reinterpret_cast<const std::byte*>(data), size);
    try {
        static_cast<void>(hh::assets::parse_hasset(bytes));
    } catch (const std::runtime_error&) {
        // Invalid binary documents are expected. Sanitizers detect unsafe failures.
    }
    return 0;
}
