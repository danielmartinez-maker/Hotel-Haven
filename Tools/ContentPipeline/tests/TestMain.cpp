#include "Test.h"
int main() {
    int failed = 0;
    for (const auto& tc : hh::assets::test::registry()) {
        try { tc.fn(); std::cout << "PASS " << tc.name << '\n'; }
        catch (const std::exception& e) { ++failed; std::cerr << "FAIL " << tc.name << ": " << e.what() << '\n'; }
    }
    return failed == 0 ? 0 : 1;
}
