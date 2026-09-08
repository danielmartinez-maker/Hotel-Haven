#include "TestFramework.h"

#include <string_view>

int main(int argc, char** argv) {
    const std::string_view filter = argc >= 2 ? std::string_view(argv[1]) : std::string_view{};
    int failures = 0;
    std::size_t executed = 0;
    for (const auto& test : hh::renderer::test::registry()) {
        if (!filter.empty() && test.name.find(filter) == std::string::npos) {
            continue;
        }
        ++executed;
        std::cout << "[RUN] " << test.name << std::endl;
        try {
            test.function();
            std::cout << "[PASS] " << test.name << std::endl;
        } catch (const std::exception& error) {
            ++failures;
            std::cerr << "[FAIL] " << test.name << " - " << error.what() << std::endl;
        } catch (...) {
            ++failures;
            std::cerr << "[FAIL] " << test.name << " - unknown exception" << std::endl;
        }
    }

    std::cout << executed << " tests, " << failures << " failures" << std::endl;
    return failures == 0 ? 0 : 1;
}
