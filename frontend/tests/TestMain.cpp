#include "TestFramework.h"

int main() {
    int failures = 0;
    for (const auto& test : hh::frontend::test::registry()) {
        try {
            test.function();
            std::cout << "[PASS] " << test.name << '\n';
        } catch (const std::exception& exception) {
            ++failures;
            std::cerr << "[FAIL] " << test.name << ": " << exception.what() << '\n';
        } catch (...) {
            ++failures;
            std::cerr << "[FAIL] " << test.name << ": unknown exception\n";
        }
    }
    std::cout << hh::frontend::test::registry().size() << " tests, " << failures << " failures\n";
    return failures == 0 ? 0 : 1;
}
