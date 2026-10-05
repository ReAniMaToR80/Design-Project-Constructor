// Project.h — result of the constructor: resolved configuration,
// specification (спецификация ТХ/КМ/ЭМ/МЗ) and detected conflicts.
#pragma once

#include "LinkMatrix.h"
#include "Types.h"

#include <string>
#include <string_view>
#include <vector>

namespace bsc {

struct SpecLine {
    SpecItem item;
    double quantity = 0.0;  // total for the site
    std::string origin;     // the link the line comes from, e.g. "МТС × Оптика"
};

using Specification = std::vector<SpecLine>;

enum class Severity : std::uint8_t { Info, Warning, Error };

std::string_view nameOf(Severity value) noexcept;

struct Conflict {
    Severity severity = Severity::Error;
    std::string code;
    std::string message;
};

struct Project {
    SiteConfiguration requested;  // as received from the act / GUI
    SiteConfiguration resolved;   // completed with the typical solution defaults
    double cableRouteLengthM = 0.0;
    Specification specification;
    std::vector<Conflict> conflicts;
    std::vector<std::string> parserWarnings;

    // No conflict with Severity::Error.
    bool isValid() const noexcept;
    bool hasConflict(std::string_view code) const noexcept;
    // Sum of quantities of all lines with this item code.
    double quantityOf(std::string_view itemCode) const noexcept;
};

}  // namespace bsc
