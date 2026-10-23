// TestRunner.h — dependency-free unit test runner (no macros, no frameworks),
// so the tests build the same way with GCC, Clang and MSVC.
#pragma once

#include <functional>
#include <sstream>
#include <string>
#include <string_view>
#include <vector>

namespace bsc::test {

class Check {
public:
    void isTrue(bool condition, std::string_view what);
    void isFalse(bool condition, std::string_view what);

    template <typename A, typename B>
    void equal(const A& actual, const B& expected, std::string_view what) {
        if (actual == expected) {
            ++passed_;
            return;
        }
        std::ostringstream message;
        message << what << ": expected [" << expected << "], got [" << actual << "]";
        fail(message.str());
    }

    void near(double actual, double expected, double tolerance, std::string_view what);

    int failures() const noexcept { return static_cast<int>(failures_.size()); }
    int passed() const noexcept { return passed_; }
    const std::vector<std::string>& messages() const noexcept { return failures_; }

private:
    void fail(std::string message);

    int passed_ = 0;
    std::vector<std::string> failures_;
};

struct TestCase {
    std::string name;
    std::function<void(Check&)> body;
};

using Registry = std::vector<TestCase>;

// Runs all tests whose name contains filter; returns the process exit code.
int runAll(const Registry& registry, std::string_view filter);

}  // namespace bsc::test
