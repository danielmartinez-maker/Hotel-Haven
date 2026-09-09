#pragma once

#include <cmath>
#include <exception>
#include <iostream>
#include <sstream>
#include <stdexcept>
#include <string>
#include <vector>

namespace hh::frontend::test {

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

}  // namespace hh::frontend::test

#define HH_FRONTEND_TEST_CONCAT_INNER(a, b) a##b
#define HH_FRONTEND_TEST_CONCAT(a, b) HH_FRONTEND_TEST_CONCAT_INNER(a, b)
#define TEST_CASE(name) \
    static void HH_FRONTEND_TEST_CONCAT(hh_frontend_test_fn_, __LINE__)(); \
    static ::hh::frontend::test::Registrar HH_FRONTEND_TEST_CONCAT(hh_frontend_test_reg_, __LINE__)(name, &HH_FRONTEND_TEST_CONCAT(hh_frontend_test_fn_, __LINE__)); \
    static void HH_FRONTEND_TEST_CONCAT(hh_frontend_test_fn_, __LINE__)()
#define EXPECT_TRUE(expression) do { if (!(expression)) ::hh::frontend::test::fail(#expression, __FILE__, __LINE__); } while (false)
#define EXPECT_FALSE(expression) EXPECT_TRUE(!(expression))
#define EXPECT_EQ(lhs, rhs) do { const auto hh_lhs = (lhs); const auto hh_rhs = (rhs); if (!(hh_lhs == hh_rhs)) ::hh::frontend::test::fail(#lhs " == " #rhs, __FILE__, __LINE__); } while (false)
#define EXPECT_NEAR(lhs, rhs, epsilon) ::hh::frontend::test::expectNear((lhs), (rhs), (epsilon), #lhs " ~= " #rhs, __FILE__, __LINE__)
