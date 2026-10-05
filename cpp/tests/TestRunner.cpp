#include "TestRunner.h"

#include <exception>
#include <iostream>
#include <utility>

namespace bsc::test {

void Check::isTrue(bool condition, std::string_view what) {
    if (condition) {
        ++passed_;
    } else {
        fail(std::string(what) + ": expected true");
    }
}

void Check::isFalse(bool condition, std::string_view what) {
    isTrue(!condition, what);
}

void Check::near(double actual, double expected, double tolerance, std::string_view what) {
    const double delta = actual > expected ? actual - expected : expected - actual;
    if (delta <= tolerance) {
        ++passed_;
        return;
    }
    std::ostringstream message;
    message << what << ": expected " << expected << " ± " << tolerance << ", got " << actual;
    fail(message.str());
}

void Check::fail(std::string message) {
    failures_.push_back(std::move(message));
}

int runAll(const Registry& registry, std::string_view filter) {
    int failedTests = 0;
    int executed = 0;
    for (const TestCase& test : registry) {
        if (!filter.empty() && test.name.find(filter) == std::string::npos) {
            continue;
        }
        ++executed;
        Check check;
        try {
            test.body(check);
        } catch (const std::exception& error) {
            check.isTrue(false, std::string("unexpected exception: ") + error.what());
        }
        if (check.failures() == 0) {
            std::cout << "[  OK  ] " << test.name << " (" << check.passed() << " checks)\n";
        } else {
            ++failedTests;
            std::cout << "[ FAIL ] " << test.name << "\n";
            for (const std::string& message : check.messages()) {
                std::cout << "         " << message << "\n";
            }
        }
    }
    std::cout << "\n" << executed - failedTests << "/" << executed << " tests passed\n";
    return (failedTests == 0 && executed > 0) ? 0 : 1;
}

}  // namespace bsc::test
