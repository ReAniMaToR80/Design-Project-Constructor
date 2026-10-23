// Validator.h — conflict checks of the resolved configuration.
#pragma once

#include "LinkMatrix.h"
#include "Project.h"
#include "Types.h"

#include <vector>

namespace bsc {

class Validator {
public:
    // Dynamic factor for wind / ice load on the antenna structure (assumption,
    // to be replaced by the strength calculation of the КМ section).
    static constexpr double kLoadSafetyFactor = 1.3;
    // Longer coaxial routes lose too much power: optics is recommended.
    static constexpr double kMaxFeederRouteM = 50.0;

    explicit Validator(const LinkMatrix& matrix) noexcept;

    // Operator regulation and position type compatibility of every chosen element.
    std::vector<Conflict> checkConfiguration(const SiteConfiguration& resolved, double cableRouteLengthM) const;

    // Bearing capacity of the КМ structure against the equipment mounted on it.
    std::vector<Conflict> checkStructureLoad(const Specification& specification) const;

private:
    const LinkMatrix& matrix_;
};

}  // namespace bsc
