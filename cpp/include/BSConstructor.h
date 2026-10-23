// BSConstructor.h — facade of the BS project constructor core.
//
// Pipeline: act text -> ActParser -> SiteConfiguration -> defaults from the
// LinkMatrix -> Validator -> Specification (ТХ / КМ / ЭМ / МЗ).
// The typical solution base is shared (std::shared_ptr<const LinkMatrix>)
// between constructors; the current project is owned (std::unique_ptr).
#pragma once

#include "ActParser.h"
#include "LinkMatrix.h"
#include "Project.h"
#include "Types.h"

#include <memory>
#include <string_view>

namespace bsc {

class BSConstructor {
public:
    // Rooftop route: from the cabinet across the roof to the antennas.
    static constexpr double kDefaultRooftopRouteM = 30.0;
    static constexpr double kDefaultGroundHeightM = 40.0;
    static constexpr double kGroundRouteReserveM = 10.0;

    // Throws std::invalid_argument if matrix is null.
    explicit BSConstructor(std::shared_ptr<const LinkMatrix> matrix = LinkMatrix::makeTypical());

    // Stateless build of a project from the requested configuration.
    Project build(const SiteConfiguration& requested) const;

    // Stateful API for the GUI: the last built project becomes current.
    const Project& loadAct(std::string_view actText);
    const Project& configure(const SiteConfiguration& requested);
    // Rebuilds the current project for another operator, keeping the site
    // data and the explicit choices. Throws std::logic_error without a project.
    const Project& switchOperator(Operator op);

    const Project* current() const noexcept;
    void reset() noexcept;

    const LinkMatrix& matrix() const noexcept;
    std::shared_ptr<const LinkMatrix> sharedMatrix() const noexcept;

    static double estimateRouteLength(const SiteConfiguration& config) noexcept;

private:
    Specification resolveSpecification(const SiteConfiguration& resolved, double routeLengthM) const;
    const Project& commit(Project project);

    std::shared_ptr<const LinkMatrix> matrix_;
    ActParser parser_;
    std::unique_ptr<Project> project_;
};

}  // namespace bsc
