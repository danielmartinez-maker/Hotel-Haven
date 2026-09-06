#pragma once
#include <functional>
#include <iostream>
#include <stdexcept>
#include <string>
#include <utility>
#include <vector>

namespace hh::assets::test {
using TestFn = std::function<void()>;
struct Case { std::string name; TestFn fn; };
inline std::vector<Case>& registry() { static std::vector<Case> r; return r; }
struct Registrar { Registrar(std::string n, TestFn f) { registry().push_back({std::move(n), std::move(f)}); } };
inline void require(bool cond, const char* expr, const char* file, int line) {
    if (!cond) throw std::runtime_error(std::string(file) + ":" + std::to_string(line) + " REQUIRE(" + expr + ") failed");
}
}
#define HH_TEST_CONCAT_INNER(a,b) a##b
#define HH_TEST_CONCAT(a,b) HH_TEST_CONCAT_INNER(a,b)
#define HH_TEST(name) \
    static void HH_TEST_CONCAT(test_fn_, __LINE__)(); \
    static ::hh::assets::test::Registrar HH_TEST_CONCAT(test_reg_, __LINE__)(name, HH_TEST_CONCAT(test_fn_, __LINE__)); \
    static void HH_TEST_CONCAT(test_fn_, __LINE__)()
#define HH_REQUIRE(expr) ::hh::assets::test::require(static_cast<bool>(expr), #expr, __FILE__, __LINE__)
