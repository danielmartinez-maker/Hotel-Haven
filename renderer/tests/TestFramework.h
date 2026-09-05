#pragma once

#include <cmath>
#include <exception>
#include <functional>
#include <iostream>
#include <sstream>
#include <stdexcept>
#include <string>
#include <utility>
#include <vector>

namespace hh::renderer::test {

using TestFunction = void (*)();

struct TestCase {
    std::string name;
    TestFunction function;
};

inline std::vector<TestCase>& registry() {
    static std::vector<TestCase> tests;
    return tests;
}

struct Registrar {
    Registrar(const char* name, TestFunction function) {
        registry().push_back(TestCase{name, function});
    }
};

[[noreturn]] inline void fail(const char* expression, const char* file, int line) {
    std::ostringstream stream;
    stream << file << ':' << line << ": expectation failed: " << expression;
    throw std::runtime_error(stream.str());
}

inline void expectNear(float lhs, float rhs, float epsilon, const char* expression, const char* file, int line) {
    if (std::fabs(lhs - rhs) > epsilon) {
        fail(expression, file, line);
    }
}

}  // namespace hh::renderer::test

#define HH_TEST_CONCAT_INNER(a, b) a##b
#define HH_TEST_CONCAT(a, b) HH_TEST_CONCAT_INNER(a, b)
#define TEST_CASE(name) \
    static void HH_TEST_CONCAT(hh_test_fn_, __LINE__)(); \
    static ::hh::renderer::test::Registrar HH_TEST_CONCAT(hh_test_reg_, __LINE__)(name, &HH_TEST_CONCAT(hh_test_fn_, __LINE__)); \
    static void HH_TEST_CONCAT(hh_test_fn_, __LINE__)()
#define EXPECT_TRUE(expression) do { if (!(expression)) ::hh::renderer::test::fail(#expression, __FILE__, __LINE__); } while (false)
#define EXPECT_FALSE(expression) EXPECT_TRUE(!(expression))
#define EXPECT_EQ(lhs, rhs) do { const auto hh_lhs = (lhs); const auto hh_rhs = (rhs); if (!(hh_lhs == hh_rhs)) ::hh::renderer::test::fail(#lhs " == " #rhs, __FILE__, __LINE__); } while (false)
#define EXPECT_NEAR(lhs, rhs, epsilon) ::hh::renderer::test::expectNear((lhs), (rhs), (epsilon), #lhs " ~= " #rhs, __FILE__, __LINE__)
