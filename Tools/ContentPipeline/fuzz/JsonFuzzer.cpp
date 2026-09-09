#include "hh/assets/Json.h"

#include <cstddef>
#include <cstdint>
#include <stdexcept>
#include <string_view>

extern "C" int LLVMFuzzerTestOneInput(const std::uint8_t* data, std::size_t size) {
    const auto text = std::string_view(reinterpret_cast<const char*>(data), size);
    try {
        static_cast<void>(hh::assets::parse_json(text));
    } catch (const std::runtime_error&) {
        // Invalid JSON is expected. Sanitizers and libFuzzer detect memory/UB failures.
    } catch (const std::out_of_range&) {
        // Defensive: parser/value helpers may reject structurally invalid documents.
    }
    return 0;
}
