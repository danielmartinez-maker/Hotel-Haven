#pragma once

#include <cstdlib>
#include <stdexcept>
#include <string>

namespace hh::renderer::stress_test {

inline std::string scaleFromEnvironment() {
#ifdef _WIN32
    char* value = nullptr;
    std::size_t length = 0;
    if (_dupenv_s(&value, &length, "HH_STRESS_SCALE") != 0)
        throw std::runtime_error("failed to read HH_STRESS_SCALE");
    const std::string scale = value && *value ? value : "pr";
    std::free(value);
    return scale;
#else
    const char* value = std::getenv("HH_STRESS_SCALE");
    return value && *value ? std::string(value) : std::string("pr");
#endif
}

}  // namespace hh::renderer::stress_test
