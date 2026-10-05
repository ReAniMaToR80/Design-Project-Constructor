#include "TestRunner.h"

#include <string_view>

namespace bsc::test {
void registerLinksTests(Registry& registry);
void registerParserTests(Registry& registry);
void registerConstructorTests(Registry& registry);
}  // namespace bsc::test

// Usage: bsc_tests [NAME_FILTER]
int main(int argc, char* argv[]) {
    bsc::test::Registry registry;
    bsc::test::registerLinksTests(registry);
    bsc::test::registerParserTests(registry);
    bsc::test::registerConstructorTests(registry);
    const std::string_view filter = argc > 1 ? std::string_view(argv[1]) : std::string_view();
    return bsc::test::runAll(registry, filter);
}
