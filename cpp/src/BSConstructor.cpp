#include "BSConstructor.h"

#include "Validator.h"

#include <cmath>
#include <stdexcept>
#include <string>
#include <utility>

namespace bsc {
namespace {

double quantityFor(const SpecItem& item, unsigned sectors, double routeLengthM) noexcept {
    const double sectorCount = static_cast<double>(sectors);
    switch (item.basis) {
        case QuantityBasis::Fixed: return item.quantity;
        case QuantityBasis::PerSector: return item.quantity * sectorCount;
        case QuantityBasis::PerRouteMeter: return item.quantity * routeLengthM;
        case QuantityBasis::PerSectorRouteMeter: return item.quantity * sectorCount * routeLengthM;
    }
    return item.quantity;
}

std::string linkName(std::string_view a, std::string_view b) {
    std::string name(a);
    name += " × ";
    name += b;
    return name;
}

void appendLines(Specification& specification, std::vector<SpecItem> items, const std::string& origin,
                 unsigned sectors, double routeLengthM) {
    for (SpecItem& item : items) {
        const double quantity = quantityFor(item, sectors, routeLengthM);
        specification.push_back(SpecLine{std::move(item), quantity, origin});
    }
}

}  // namespace

BSConstructor::BSConstructor(std::shared_ptr<const LinkMatrix> matrix) : matrix_(std::move(matrix)) {
    if (!matrix_) {
        throw std::invalid_argument("BSConstructor: link matrix is required");
    }
}

Project BSConstructor::build(const SiteConfiguration& requested) const {
    Project project;
    project.requested = requested;
    project.resolved = requested;
    SiteConfiguration& resolved = project.resolved;
    std::vector<Conflict>& conflicts = project.conflicts;

    if (resolved.siteId.empty()) {
        conflicts.push_back({Severity::Error, "E_NO_SITE_ID", "Не определен номер БС"});
    }
    if (!resolved.op) {
        conflicts.push_back({Severity::Error, "E_NO_OPERATOR", "Не определен оператор"});
    }
    if (!resolved.position) {
        conflicts.push_back({Severity::Error, "E_NO_POSITION", "Не определен тип площадки (крышная / наземная АМС)"});
    }
    if (resolved.sectors == 0) {
        conflicts.push_back({Severity::Error, "E_NO_SECTORS", "Количество секторов должно быть больше нуля"});
    }

    if (resolved.position) {
        for (const Aspect aspect : kAllAspects) {
            if (resolved.choice(aspect)) {
                continue;
            }
            const auto fallback = matrix_->defaultFor(*resolved.position, aspect);
            if (fallback) {
                resolved.setChoice(aspect, fallback);
                conflicts.push_back({Severity::Info, "I_DEFAULT_APPLIED",
                                     std::string(nameOf(aspect)) + ": принято типовое решение «" +
                                         std::string(nameOf(*fallback)) + "»"});
            } else {
                conflicts.push_back({Severity::Error, "E_NO_CHOICE",
                                     std::string(nameOf(aspect)) + ": нет выбора и нет типового решения"});
            }
        }
    }

    project.cableRouteLengthM = estimateRouteLength(resolved);

    const Validator validator(*matrix_);
    for (Conflict& conflict : validator.checkConfiguration(resolved, project.cableRouteLengthM)) {
        conflicts.push_back(std::move(conflict));
    }
    if (resolved.op && resolved.position) {
        project.specification = resolveSpecification(resolved, project.cableRouteLengthM);
        for (Conflict& conflict : validator.checkStructureLoad(project.specification)) {
            conflicts.push_back(std::move(conflict));
        }
    }
    return project;
}

const Project& BSConstructor::loadAct(std::string_view actText) {
    ParseResult parsed = parser_.parse(actText);
    Project project = build(parsed.config);
    project.parserWarnings = std::move(parsed.warnings);
    return commit(std::move(project));
}

const Project& BSConstructor::configure(const SiteConfiguration& requested) {
    return commit(build(requested));
}

const Project& BSConstructor::switchOperator(Operator op) {
    if (!project_) {
        throw std::logic_error("BSConstructor: no current project to switch the operator for");
    }
    SiteConfiguration requested = project_->requested;
    requested.op = op;
    Project project = build(requested);
    project.parserWarnings = project_->parserWarnings;
    return commit(std::move(project));
}

const Project* BSConstructor::current() const noexcept {
    return project_.get();
}

void BSConstructor::reset() noexcept {
    project_.reset();
}

const LinkMatrix& BSConstructor::matrix() const noexcept {
    return *matrix_;
}

std::shared_ptr<const LinkMatrix> BSConstructor::sharedMatrix() const noexcept {
    return matrix_;
}

double BSConstructor::estimateRouteLength(const SiteConfiguration& config) noexcept {
    double length = 0.0;
    if (config.cableRouteLengthM) {
        length = *config.cableRouteLengthM;
    } else if (config.position == PositionType::GroundMast) {
        length = config.antennaHeightM.value_or(kDefaultGroundHeightM) + kGroundRouteReserveM;
    } else {
        length = kDefaultRooftopRouteM;
    }
    return std::ceil(length);
}

Specification BSConstructor::resolveSpecification(const SiteConfiguration& resolved, double routeLengthM) const {
    Specification specification;
    const Operator op = *resolved.op;
    const PositionType position = *resolved.position;
    const unsigned sectors = resolved.sectors;

    appendLines(specification, matrix_->itemsFor(op, position), linkName(nameOf(op), nameOf(position)), sectors,
                routeLengthM);
    for (const Aspect aspect : kAllAspects) {
        const auto element = resolved.choice(aspect);
        if (!element) {
            continue;
        }
        appendLines(specification, matrix_->itemsFor(op, *element), linkName(nameOf(op), nameOf(*element)),
                    sectors, routeLengthM);
        appendLines(specification, matrix_->itemsFor(position, *element),
                    linkName(nameOf(position), nameOf(*element)), sectors, routeLengthM);
    }
    return specification;
}

const Project& BSConstructor::commit(Project project) {
    auto next = std::make_unique<Project>(std::move(project));
    project_ = std::move(next);
    return *project_;
}

}  // namespace bsc
