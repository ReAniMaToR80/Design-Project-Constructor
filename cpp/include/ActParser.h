// ActParser.h — extraction of the site data from the act text (акт ППО).
//
// Accepted input: "Ключ: значение" / "Ключ = значение" lines, "|"-separated
// segments in one line (tracker summary format) and JSON-like
// "Site_ID": "78-04546", lines produced by the input_parser script.
// The site number and the operator are also searched in free text.
#pragma once

#include "Types.h"

#include <cstddef>
#include <optional>
#include <string>
#include <string_view>
#include <vector>

namespace bsc {

struct ParseResult {
    SiteConfiguration config;
    std::vector<std::string> warnings;
};

class ActParser {
public:
    static constexpr std::size_t kMaxActBytes = 1024 * 1024;
    static constexpr std::size_t kMaxSiteIdBytes = 64;
    static constexpr unsigned kMaxSectors = 12;

    ParseResult parse(std::string_view text) const;
    // nullptr-safe overload for buffers coming from C APIs / GUI controls.
    ParseResult parse(const char* data, std::size_t size) const;

    static std::optional<Operator> detectOperator(std::string_view value);
    static std::optional<PositionType> detectPositionType(std::string_view value);
    static std::optional<Element> detectElement(std::string_view value, Aspect aspect);
    // "БС №78-04546 (Орово)" -> "78-04546"; empty if no identifier is found.
    static std::string extractSiteId(std::string_view value);
};

}  // namespace bsc
